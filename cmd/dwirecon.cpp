/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include <algorithm>

#include "command.h"
#include "image.h"
#include "math/SH.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "adapter/extract.h"
#include "dwi/svr/recon.h"

#define DEFAULT_LMAX 4
#define DEFAULT_TOL 1e-4
#define DEFAULT_MAXITER 100

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Daan Christiaens";

  SYNOPSIS = "Reconstruct DWI signal from a series of scattered slices with associated motion parameters.";

  DESCRIPTION
  + "";

  ARGUMENTS
  + Argument ("DWI", "the input DWI image.").type_image_in()
  + Argument ("SH", "the output spherical harmonics coefficients image.").type_image_out();


  OPTIONS
  + Option ("motion", "The motion parameters associated with input slices or volumes. "
                      "These are supplied as a matrix of 6 columns that encode respectively "
                      "the x-y-z translation and 0-1-2 rotation Euler angles for each volume "
                      "or slice in the image. All transformations are w.r.t. scanner space." )
    + Argument ("file").type_file_in()

  + Option ("rf", "Basis functions for the radial (multi-shell) domain, provided as matrices in which "
                  "rows correspond with shells and columns with SH harmonic bands.").allow_multiple()
    + Argument ("b").type_file_in()
  
  + Option ("lmax",
            "set the maximum harmonic order for the output series. (default = " + str(DEFAULT_LMAX) + ")")
    + Argument ("order").type_integer(0, 30)

  + Option ("weights",
            "Slice weights, provided as a matrix of dimensions Nslices x Nvols.")
    + Argument ("W").type_file_in()

  + DWI::GradImportOptions()

  + DWI::ShellOption

  + OptionGroup ("Output options")

  + Option ("rpred",
            "output predicted signal in original (rotated) directions. (useful for registration)")
    + Argument ("out").type_image_out()

  + Option ("spred",
            "output source prediction of all scattered slices. (useful for diagnostics)")
    + Argument ("out").type_image_out()

  + Option ("tpred",
            "output predicted signal in the space of the target reconstruction.")
    + Argument ("out").type_image_out()

  + OptionGroup ("CG Optimization options")

  + Option ("tolerance", "the tolerance on the conjugate gradient solver. (default = " + str(DEFAULT_TOL) + ")")
    + Argument ("t").type_float(0.0, 1.0)

  + Option ("maxiter",
            "the maximum number of iterations of the conjugate gradient solver. (default = " + str(DEFAULT_MAXITER) + ")")
    + Argument ("n").type_integer(1);

}


typedef float value_type;



void run ()
{
  // Load input data
  auto dwi = Image<value_type>::open(argument[0]);

  // Read motion parameters
  auto opt = get_options("motion");
  Eigen::MatrixXf motion;
  if (opt.size())
    motion = load_matrix<float>(opt[0][0]);
  else
    motion = Eigen::MatrixXf::Zero(dwi.size(3), 6); 

  // Check dimensions
  if (motion.size() && motion.cols() != 6)
    throw Exception("No. columns in motion parameters must equal 6.");
  if (motion.size() && (motion.rows() != dwi.size(3)) && (motion.rows() != dwi.size(3) * dwi.size(2)))
    throw Exception("No. rows in motion parameters must equal the number of DWI volumes or slices.");


  // Select shells
  auto grad = DWI::get_valid_DW_scheme (dwi);
  DWI::Shells shells (grad);
  shells.select_shells (false, false, false);

  // Read multi-shell basis
  int lmax = 0;
  vector<Eigen::MatrixXf> rf;
  opt = get_options("rf");
  for (size_t k = 0; k < opt.size(); k++) {
    Eigen::MatrixXf t = load_matrix<float>(opt[k][0]);
    if (t.rows() != shells.count())
      throw Exception("No. shells does not match no. rows in basis function " + opt[k][0] + ".");
    lmax = std::max(2*(int(t.cols())-1), lmax);
    rf.push_back(t);
  }

  // Read slice weights
  Eigen::MatrixXf W = Eigen::MatrixXf::Ones(dwi.size(2), dwi.size(3));
  opt = get_options("weights");
  if (opt.size()) {
    W = load_matrix<float>(opt[0][0]);
    if (W.rows() != dwi.size(2) || W.cols() != dwi.size(3))
      throw Exception("Weights marix dimensions don't match image dimensions.");
  }
  //Eigen::VectorXf Wm = W.rowwise().mean();   // Normalise slice weights across volumes.
  //W.array().colwise() /= Wm.array();         // (not technically needed, but useful preconditioning for CG)

  // Get volume indices 
  vector<size_t> idx;
  if (rf.empty()) {
    idx = shells.largest().get_volumes();
  } else {
    for (size_t k = 0; k < shells.count(); k++)
      idx.insert(idx.end(), shells[k].get_volumes().begin(), shells[k].get_volumes().end());
    std::sort(idx.begin(), idx.end());
  }

  // Select subset
  auto dwisub = Adapter::make <Adapter::Extract1D> (dwi, 3, container_cast<vector<int>> (idx));

  Eigen::MatrixXf gradsub (idx.size(), grad.cols());
  for (size_t i = 0; i < idx.size(); i++)
    gradsub.row(i) = grad.row(idx[i]).template cast<float>();

  Eigen::MatrixXf motionsub;
  if (motion.rows() == dwi.size(3)) {   // per-volume rigid motion
    motionsub.resize(idx.size(), motion.cols());
    for (size_t i = 0; i < idx.size(); i++)
      motionsub.row(i) = motion.row(idx[i]).template cast<float>();
  }
  else {                                // per-slice rigid motion
    motionsub.resize(idx.size() * dwi.size(2), motion.cols());
    for (size_t i = 0; i < idx.size(); i++)
      for (size_t j = 0; j < dwi.size(2); j++)
        motionsub.row(i * dwi.size(2) + j) = motion.row(idx[i] * dwi.size(2) + j).template cast<float>();
  }

  Eigen::MatrixXf Wsub (W.rows(), idx.size());
  for (size_t i = 0; i < idx.size(); i++)
    Wsub.col(i) = W.col(idx[i]);

  // Other parameters
  if (rf.empty())
    lmax = get_option_value("lmax", DEFAULT_LMAX);
  else
    lmax = std::min(lmax, (int) get_option_value("lmax", lmax));
  
  value_type tol = get_option_value("tolerance", DEFAULT_TOL);
  size_t maxiter = get_option_value("maxiter", DEFAULT_MAXITER);


  // Set up scattered data matrix
  INFO("initialise reconstruction matrix");
  DWI::ReconMatrix R (dwisub, motionsub, gradsub, lmax, rf);
  R.setW(Wsub);

  // Read input data to vector
  Eigen::VectorXf y (dwisub.size(0)*dwisub.size(1)*dwisub.size(2)*dwisub.size(3));
  size_t j = 0, v = 0;
  for (auto l0 = Loop("loading image data", 3)(dwisub); l0; l0++, v++) {
    size_t s = 0;
    for (auto l1 = Loop(2)(dwisub); l1; l1++, s++) {
      for (auto l = Loop({0, 1})(dwisub); l; l++, j++)
        y[j] = Wsub(s, v) * dwisub.value();
    }
  }

  // Fit scattered data in basis...
  INFO("solve with conjugate gradient method");

  //Eigen::initParallel();
  //Eigen::setNbThreads(Thread::number_of_threads());     // only used when configured with OpenMP
  //VAR(Eigen::nbThreads());

  Eigen::LeastSquaresConjugateGradient<DWI::ReconMatrix, Eigen::IdentityPreconditioner> lscg;
  lscg.compute(R);

  lscg.setTolerance(tol);
  lscg.setMaxIterations(maxiter);

  Eigen::VectorXf x = lscg.solve(y);

  std::cout << "LSCG: #iterations: " << lscg.iterations() << ", estimated error: " << lscg.error() << std::endl;


  // Write result to output file
  Header header (dwisub);
  header.size(3) = R.getY().cols();
  Stride::set_from_command_line (header, Stride::contiguous_along_axis (3));
  header.datatype() = DataType::from_command_line (DataType::Float32);
  auto out = Image<value_type>::create (argument[1], header);

  j = 0;
  for (auto l = Loop("writing result to image", {0, 1, 2, 3})(out); l; l++, j++) {
    out.value() = x[j];
  }


  // Output registration prediction
  opt = get_options("rpred");
  if (opt.size()) {
    header.size(3) = motionsub.rows();
    Stride::set (header, Stride::contiguous_along_spatial_axes (header));
    auto rpred = Image<value_type>::create(opt[0][0], header);
    class PredFunctor {
    public:
      PredFunctor (const Eigen::VectorXf& _y) : y(_y) {}
      void operator () (Image<value_type>& in, Image<value_type>& out) {
        v = in.row(3);
        out.value() = y.dot(v);
      }
    private:
      Eigen::VectorXf y, v;
    };
    j = 0;
    size_t n = (motionsub.rows() == dwisub.size(3)) ? dwisub.size(2) : 1;
    for (auto l = Loop("saving registration prediction", 3)(rpred); l; l++, j+=n) {
      ThreadedLoop(out, 0, 3).run( PredFunctor (R.getY().row(j)) , out , rpred );
    }
  }


  // Output source prediction
  opt = get_options("spred");
  if (opt.size()) {
    header.size(3) = dwisub.size(3);
    DWI::stash_DW_scheme (header, gradsub);
    auto spred = Image<value_type>::create(opt[0][0], header);
    R.setW(Eigen::MatrixXf::Ones(dwisub.size(2), dwisub.size(3)));
    Eigen::VectorXf p = R * x;
    j = 0;
    for (auto l = Loop("saving source prediction", {0, 1, 2, 3})(spred); l; l++, j++) {
      spred.value() = p[j];
    }
  }


  // Output target prediction
  opt = get_options("tpred");
  if (opt.size()) {
    header.size(3) = dwisub.size(3);
    DWI::stash_DW_scheme (header, gradsub);
    Stride::set (header, Stride::contiguous_along_spatial_axes (header));
    auto tpred = Image<value_type>::create(opt[0][0], header);
    class PredFunctor {
    public:
      PredFunctor (const Eigen::VectorXf& _y) : y(_y) {}
      void operator () (Image<value_type>& in, Image<value_type>& out) {
        v = in.row(3);
        out.value() = y.dot(v);
      }
    private:
      Eigen::VectorXf y, v;
    };
    j = 0;
    for (auto l = Loop("saving registration prediction", 3)(tpred); l; l++, j++) {
      ThreadedLoop(out, 0, 3).run( PredFunctor (R.getY0(gradsub).row(j)) , out , tpred );
    }
  }


}




