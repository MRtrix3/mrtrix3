/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "command.h"
#include "image.h"
#include "memory.h"
#include "progressbar.h"
#include "algo/threaded_copy.h"
#include "algo/threaded_loop.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "dwi/directions/predefined.h"
#include "dwi/sdeconv/csd.h"
#include "dwi/sdeconv/msmt_csd.h"
#include "math/SH.h"


using namespace MR;
using namespace App;


const char* const algorithms[] = { "csd", "msmt_csd", NULL };



OptionGroup CommonOptions = OptionGroup ("Options common to more than one algorithm")

    + Option ("directions",
              "specify the directions over which to apply the non-negativity constraint "
              "(by default, the built-in 300 direction set is used). These should be "
              "supplied as a text file containing [ az el ] pairs for the directions.")
      + Argument ("file").type_file_in()

    + Option ("lmax",
              "the maximum spherical harmonic order for the output FOD(s)."
              "For algorithms with multiple outputs, this should be "
              "provided as a comma-separated list of integers, one for "
              "each output image; for single-output algorithms, only "
              "a single integer should be provided. If omitted, the "
              "command will use the highest possible lmax given the "
              "diffusion gradient table, up to a maximum of 8.")
      + Argument ("order").type_sequence_int()

    + Option ("mask",
              "only perform computation within the specified binary brain mask image.")
      + Argument ("image").type_image_in();




void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com) and Ben Jeurissen (ben.jeurissen@uantwerpen.be)";

  DESCRIPTION
    + "estimate fibre orientation distributions from diffusion data using spherical deconvolution."

    + Math::SH::encoding_description;

  REFERENCES
    + "* If using csd algorithm:\n"
    "Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "Robust determination of the fibre orientation distribution in diffusion MRI: "
    "Non-negativity constrained super-resolved spherical deconvolution. "
    "NeuroImage, 2007, 35, 1459-1472"

    + "* If using msmt_csd algorithm:\n"
    "Jeurissen, B; Tournier, J-D; Dhollander, T; Connelly, A & Sijbers, J. " // Internal
    "Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data "
    "NeuroImage, 2014, 103, 411-426"

    // TODO Explicit sd algorithm?
    + "Tournier, J.-D.; Calamante, F., Gadian, D.G. & Connelly, A. " // Internal
    "Direct estimation of the fiber orientation density function from "
    "diffusion-weighted MRI data using spherical deconvolution."
    "NeuroImage, 2004, 23, 1176-1185";

  ARGUMENTS
    + Argument ("algorithm", "the algorithm to use for FOD estimation. "
                             "(options are: " + join(algorithms, ",") + ")").type_choice (algorithms)
    + Argument ("dwi", "the input diffusion-weighted image").type_image_in()
    + Argument ("response odf", "pairs of input tissue response and output ODF images").allow_multiple();

  OPTIONS
    + DWI::GradImportOptions()
    + DWI::ShellOption
    + CommonOptions
    + DWI::SDeconv::CSD_options
    + Stride::Options;
}



class CSD_Processor
{
  public:
    CSD_Processor (const DWI::SDeconv::CSD::Shared& shared, Image<bool>& mask) :
      sdeconv (shared),
      data (shared.dwis.size()),
      mask (mask) { }

    void operator () (Image<float>& dwi, Image<float>& fod) {
      if (!load_data (dwi)) {
        for (auto l = Loop (3) (fod); l; ++l)
          fod.value() = 0.0;
        return;
      }

      sdeconv.set (data);

      size_t n;
      for (n = 0; n < sdeconv.shared.niter; n++)
        if (sdeconv.iterate())
          break;

      if (sdeconv.shared.niter && n >= sdeconv.shared.niter)
        INFO ("voxel [ " + str (dwi.index(0)) + " " + str (dwi.index(1)) + " " + str (dwi.index(2)) +
            " ] did not reach full convergence");

      write_back (fod);
    }



  private:
    DWI::SDeconv::CSD sdeconv;
    Eigen::VectorXd data;
    Image<bool> mask;


    bool load_data (Image<float>& dwi) {
      if (mask.valid()) {
        assign_pos_of (dwi, 0, 3).to (mask);
        if (!mask.value())
          return false;
      }

      for (size_t n = 0; n < sdeconv.shared.dwis.size(); n++) {
        dwi.index(3) = sdeconv.shared.dwis[n];
        data[n] = dwi.value();
        if (!std::isfinite (data[n]))
          return false;
        if (data[n] < 0.0)
          data[n] = 0.0;
      }

      return true;
    }



    void write_back (Image<float>& fod) {
      for (auto l = Loop (3) (fod); l; ++l)
        fod.value() = sdeconv.FOD() [fod.index(3)];
    }

};




class MSMT_Processor
{
  public:
    MSMT_Processor (const DWI::SDeconv::MSMT_CSD& shared, Image<bool>& mask_image, std::vector< Image<float> > odf_images) :
        shared (shared),
        solver (shared.problem),
        mask_image (mask_image),
        odf_images (odf_images),
        dwi (shared.problem.H.rows()),
        p (shared.problem.H.cols()) { }

    void operator() (Image<float>& dwi_image)
    {
      if (mask_image.valid()) {
        assign_pos_of (dwi_image, 0, 3).to (mask_image);
        if (!mask_image.value())
          return;
      }

      for (auto l = Loop (3) (dwi_image); l; ++l)
        dwi[dwi_image.index(3)] = dwi_image.value();

      size_t niter = solver (p, dwi);
      if (niter >= shared.problem.max_niter) {
        INFO ("failed to converge");
      }

      for (size_t i = 0; i < odf_images.size(); ++i) {
        assign_pos_of (dwi_image, 0, 3).to (odf_images[i]);
        for (auto l = Loop(3)(odf_images[i]); l; ++l)
          odf_images[i].value() = p[size_t(odf_images[i].index(3))];
      }
    }

  private:
    const DWI::SDeconv::MSMT_CSD& shared;
    Math::ICLS::Solver<double> solver;
    Image<bool> mask_image;
    std::vector< Image<float> > odf_images;
    Eigen::VectorXd dwi;
    Eigen::VectorXd p;
};







void run ()
{


  auto dwi = Image<float>::open (argument[1]).with_direct_io (3);
  Header header (dwi);
  header.set_ndim (4);
  header.datatype() = DataType::Float32;
  Stride::set_from_command_line (header);

  auto mask = Image<bool>();
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = Header::open (opt[0][0]).get_image<bool>();
    check_dimensions (dwi, mask, 0, 3);
  }

  int algorithm = argument[0];
  if (algorithm == 0) {

    if (argument.size() != 4)
      throw Exception ("CSD algorithm expects a single input response function and single output FOD image");

    DWI::SDeconv::CSD::Shared shared (dwi);
    shared.parse_cmdline_options();
    shared.set_response (argument[2]);
    shared.init();

    header.size(3) = shared.nSH();
    auto fod = Image<float>::create (argument[3], header);

    CSD_Processor processor (shared, mask);
    ThreadedLoop ("performing constrained spherical deconvolution", dwi, 0, 3)
        .run (processor, dwi, fod);

  } else if (algorithm == 1) {

    if (argument.size() % 2)
      throw Exception ("MSMT_CSD algorithm expects pairs of (input response function & output FOD image) to be provided");

    auto grad = DWI::get_valid_DW_scheme (dwi);

    DWI::Shells shells (grad);
    size_t nbvals = shells.count();

    std::vector<int> lmax;
    std::vector<Eigen::MatrixXd> response;
    std::vector<std::string> path;
    for (size_t i = 0; i < (argument.size()-2)/2; i++) {
      Eigen::MatrixXd r;
      try {
        r = load_matrix<> (argument[i*2+2]);
      } catch (Exception& e) {
        e.display();
        throw Exception ("The first of each argument pair should be an input response function file");
      }
      if (size_t(r.rows()) != nbvals)
        throw Exception ("number of rows in response function text file should match number of shells in dwi");
      size_t n = 0;
      for (size_t j = 0; j < size_t(r.rows()); j++) {
        for (size_t k = 0; k < size_t(r.cols()); k++) {
          if (r(j,k))
            n = std::max (n, k+1);
        }
      }
      r.conservativeResize (r.rows(), n);
      lmax.push_back ((r.cols()-1)*2);
      response.push_back (r);
      path.push_back (argument[i*2+3]);
    }

    auto opt = get_options ("lmax");
    if (opt.size()) {
      std::vector<int> lmax_user = parse_ints (opt[0][0]);
      if (lmax_user.size() != lmax.size())
        throw Exception ("Number of lmaxes specified does not match number of tissues");
      for (size_t i = 0; i < lmax_user.size(); i++) {
        if (lmax_user[i] < 0 || lmax_user[i] % 2)
          throw Exception ("Each value of lmax must be a non-negative even integer");
        lmax[i] = lmax_user[i];
        response[i].conservativeResizeLike (Eigen::MatrixXd::Zero(response[i].rows(), lmax_user[i]/2+1));
      }
    }

    Eigen::MatrixXd HR_dirs = DWI::Directions::electrostatic_repulsion_300();
    opt = get_options ("directions");
    if (opt.size())
      HR_dirs = load_matrix<> (opt[0][0]);

    DWI::SDeconv::MSMT_CSD shared (lmax, response, grad, HR_dirs);

    std::vector< Image<float> > odfs;
    for (size_t i = 0; i < path.size(); ++i) {
      header.size (3) = Math::SH::NforL (lmax[i]);
      odfs.push_back(Image<float> (Image<float>::create (path[i], header)));
    }

    MSMT_Processor processor (shared, mask, odfs);
    ThreadedLoop ("performing multi-shell, multi-tissue CSD", dwi, 0, 3)
        .run (processor, dwi);

  } else {
    assert (0);
  }

}

