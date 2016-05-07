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
#include "progressbar.h"
#include "image.h"
#include "algo/threaded_copy.h"
#include "dwi/gradient.h"
#include "dwi/directions/predefined.h"
#include "math/SH.h"
#include "math/constrained_least_squares.h"


using namespace MR;
using namespace App;
using namespace std;

typedef double value_type;


void usage ()
{

  DESCRIPTION
  + "Multi-shell, multi-tissue constrained spherical deconvolution.";

  REFERENCES
  + "Jeurissen, B; Tournier, J-D; Dhollander, T; Connelly, A & Sijbers, J. " // Internal
    "Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data "
    "NeuroImage, 2014, 103, 411-426";

  ARGUMENTS
  + Argument ("dwi", "the input dwi image.").type_image_in ()
  + Argument ("response odf",
      "the input tissue response and the output ODF image.").allow_multiple();

  OPTIONS 
  + Option ("mask", "only perform computation within the specified binary brain mask image.")
  +   Argument ("image").type_image_in()

  + Option ("lmax", "the lmax values to use per tissue type, as a comma-separated list of even integers.")
  +   Argument ("order").type_sequence_int()

  + Option ("directions",
            "specify the directions over which to apply the non-negativity constraint "
            "(by default, the built-in 300 direction set is used). These should be "
            "supplied as a text file containing the [ az el ] pairs for the directions.")
  +   Argument ("file").type_file_in()

  + DWI::GradImportOptions();
  
  AUTHOR = "Ben Jeurissen (ben.jeurissen@uantwerpen.be)";
  
  COPYRIGHT = "Copyright (C) 2015 Vision Lab, University of Antwerp, Belgium. "
    "This is free software; see the source for copying conditions. "
    "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.";
}







class Shared {
  public:
    Shared (std::vector<int>& lmax_, std::vector<Eigen::MatrixXd>& response_, Eigen::MatrixXd& grad_, Eigen::MatrixXd& HR_dirs) :
      lmax(lmax_),
      response(response_),
      grad(grad_)
  {
    DWI::Shells shells (grad);
    const size_t nbvals = shells.count();
    const size_t nsamples = grad.rows();
    const size_t ntissues = lmax.size();
    size_t nparams = 0;

    int maxlmax = 0;
    for(size_t i = 0; i < lmax.size(); i++) {
      nparams += Math::SH::NforL (lmax[i]);
      if (lmax[i] > maxlmax)
        maxlmax = lmax[i];
    }
    Eigen::MatrixXd C (nsamples, nparams);

    INFO ("initialising multi-tissue CSD for " + str(ntissues) + " tissue types, with " + str (nparams) + " parameters");
    
    std::vector<size_t> dwilist;
    for (size_t i = 0; i < nsamples; i++)
      dwilist.push_back(i);
    
    Eigen::MatrixXd directions = DWI::gen_direction_matrix (grad, dwilist);
    Eigen::MatrixXd SHT = Math::SH::init_transform (directions, maxlmax);
    for (ssize_t i = 0; i < SHT.rows(); i++)
      for (ssize_t j = 0; j < SHT.cols(); j++)
        if (std::isnan (SHT(i,j)))
          SHT(i,j) = 0.0;
    
    // TODO: is this just computing the Associated Legrendre polynomials...?
    Eigen::MatrixXd delta(1,2); delta << 0, 0;
    Eigen::MatrixXd DSH__ = Math::SH::init_transform (delta, maxlmax);
    Eigen::VectorXd DSH_ = DSH__.row(0);
    Eigen::VectorXd DSH (maxlmax/2+1);
    size_t j = 0;
    for (ssize_t i = 0; i < DSH_.size(); i++)
      if (DSH_[i] != 0.0) {
        DSH[j] = DSH_[i];
        j++;
      }

    size_t pbegin = 0;
    for (size_t tissue_idx = 0; tissue_idx < ntissues; ++tissue_idx) {
      const size_t tissue_lmax = lmax[tissue_idx];
      const size_t tissue_n = Math::SH::NforL (tissue_lmax);
      const size_t tissue_nmzero = tissue_lmax/2+1;

      for (size_t shell_idx = 0; shell_idx < nbvals; ++shell_idx) {
        Eigen::VectorXd response_ = response[tissue_idx].row(shell_idx);
        response_ = (response_.array()/DSH.head(tissue_nmzero).array()).matrix();
        Eigen::VectorXd fconv(tissue_n);
        int li = 0; int mi = 0;
        for (int l = 0; l <= int(tissue_lmax); l+=2) {
          for (int m = -l; m <= l; m++) {
            fconv[mi] = response_[li];
            mi++;
          }
          li++;
        }
        std::vector<size_t> vols = shells[shell_idx].get_volumes();
        for (size_t idx = 0; idx < vols.size(); idx++) {
          Eigen::VectorXd SHT_(SHT.row(vols[idx]).head(tissue_n));
          SHT_ = (SHT_.array()*fconv.array()).matrix();
          C.row(vols[idx]).segment(pbegin,tissue_n) = SHT_;
        }
      }
      pbegin+=tissue_n;
    }
    
    std::vector<size_t> m(lmax.size());
    std::vector<size_t> n(lmax.size());
    size_t M = 0;
    size_t N = 0;

    Eigen::MatrixXd SHT300 = Math::SH::init_transform (HR_dirs, maxlmax);

    for(size_t i = 0; i < lmax.size(); i++) {
      if (lmax[i] > 0)
        m[i] = HR_dirs.rows();
      else
        m[i] = 1;
      M += m[i];
      n[i] = Math::SH::NforL (lmax[i]);
      N += n[i];
    }

    Eigen::MatrixXd A (M,N);
    size_t b_m = 0; size_t b_n = 0;
    for(size_t i = 0; i < lmax.size(); i++) {
      A.block(b_m,b_n,m[i],n[i]) = SHT300.block(0,0,m[i],n[i]);
      b_m += m[i];
      b_n += n[i];
    }

    problem = Math::ICLS::Problem<value_type>(C, A, 1.0e-10, 1.0e-10);
  };

  public:
    std::vector<int> lmax;
    std::vector<Eigen::MatrixXd> response;
    Eigen::MatrixXd& grad;
    Math::ICLS::Problem<value_type> problem;
};







template <class MASKType, class ODFType> 
class Processor
{
  public:
    Processor (const Shared& shared, MASKType* mask_image, std::vector<ODFType> odf_images) :
      shared (shared),
      solver (shared.problem),
      mask_image (mask_image),
      odf_images (odf_images),
      dwi(shared.problem.H.rows()),
      p(shared.problem.H.cols()) { }

    template <class DWIType>
      void operator() (DWIType& dwi_image)
      {
        if (mask_image) {
          assign_pos_of (dwi_image, 0, 3).to (*mask_image);
          if (!mask_image->value())
            return;
        }
        
        for (auto l = Loop (3) (dwi_image); l; ++l)
          dwi[dwi_image.index(3)] = dwi_image.value();
        
        size_t niter = solver (p, dwi);
        if (niter >= shared.problem.max_niter) {
          INFO ("failed to converge");
        }
        
        size_t j = 0;
        for (size_t i = 0; i < odf_images.size(); ++i) {
          assign_pos_of (dwi_image, 0, 3).to (odf_images[i]);
          for (auto l = Loop(3)(odf_images[i]); l; ++l)
            odf_images[i].value() = p[j++];
        }
      }

  private:
    const Shared& shared;
    Math::ICLS::Solver<value_type> solver;
    copy_ptr<MASKType> mask_image;
    std::vector<ODFType> odf_images;
    Eigen::VectorXd dwi;
    Eigen::VectorXd p;
};




template <class MASKType, class ODFType> 
inline Processor<MASKType, ODFType>  processor (const Shared& shared, MASKType* mask_image, std::vector<ODFType> odf_images) {
  return { shared, mask_image, odf_images };
}






void run ()
{
  auto dwi = Header::open (argument[0]).get_image<value_type>();
  auto grad = DWI::get_valid_DW_scheme (dwi);
  
  DWI::Shells shells (grad);
  size_t nbvals = shells.count();
 
  std::vector<int> lmax;
  std::vector<Eigen::MatrixXd> response;
  for (size_t i = 0; i < (argument.size()-1)/2; i++) {
    Eigen::MatrixXd r = load_matrix<> (argument[i*2+1]);
    size_t n = 0;
    for (size_t j = 0; j < size_t(r.rows()); j++) {
      for (size_t k = 0; k < size_t(r.cols()); k++) {
        if (r(j,k) && (k + 1 > n)) {
          n = k + 1;
        }
      }
    }
    r.conservativeResize(r.rows(),n);
    if (size_t(r.rows()) != nbvals)
      throw Exception ("number of rows in response function text file should match number of shells in dwi");
    response.push_back(r);
    lmax.push_back((r.cols()-1)*2);
  }
    
  auto opt = get_options ("lmax");
  if (opt.size()) {
    std::vector<int> lmax_in = opt[0][0];
    if (lmax_in.size() == lmax.size()) {
      for (size_t i = 0; i<lmax_in.size(); i++) {
        if (lmax_in[i] < 0 || lmax_in[i] > 30) {
          throw Exception ("lmaxes should be even and between 0 and 30");
        }
        lmax[i] = lmax_in[i];
      }
    } else {
      throw Exception ("number of lmaxes does not match number of response functions");
    }
  }
  
  std::vector<size_t> nparams;
  for(size_t i = 0; i < lmax.size(); i++) {
    nparams.push_back(Math::SH::NforL(lmax[i]));
    response[i].conservativeResize(response[i].rows(),lmax[i]/2+1);
  }
    
  Eigen::MatrixXd HR_dirs = DWI::Directions::electrostatic_repulsion_300();
  opt = get_options ("directions");
  if (opt.size())
    HR_dirs = load_matrix<> (opt[0][0]);

  Shared shared (lmax,response,grad,HR_dirs);  
  
  Image<bool>* mask = nullptr;
  opt = get_options ("mask");
  if (opt.size()) {
    mask = new Image<bool> (Image<bool>::open (opt[0][0]));
    check_dimensions (dwi, *mask, 0, 3);
  }
  
  Header header (dwi);
  header.datatype() = DataType::Float32;
  header.set_ndim (4);
  
  std::vector<Image<value_type> > odfs;
  for (size_t i = 0; i < (argument.size()-1)/2; ++i) {
    header.size (3) = nparams[i];
    odfs.push_back(Image<value_type> (Image<value_type>::create (argument[(i+1)*2], header)));
  }
  
  ThreadedLoop ("computing", dwi, 0, 3).run (processor(shared, mask, odfs), dwi);
}

