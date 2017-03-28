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
  + Option ("lmax",
            "set the maximum harmonic order for the output series. (default = " + str(DEFAULT_LMAX) + ")")
    + Argument ("order").type_integer(0, 30)

  + Option ("motion", "The motion parameters associated with input slices or volumes. "
                      "These are supplied as a matrix of 6 columns that encode respectively "
                      "the x-y-z translation and 0-1-2 rotation Euler angles for each volume "
                      "or slice in the image. All transformations are w.r.t. scanner space." )
    + Argument ("file").type_file_in()

  + DWI::GradImportOptions()

  + DWI::ShellOption

  + Option ("tolerance", "the tolerance on the conjugate gradient solver. (default = " + str(DEFAULT_TOL) + ")")
    + Argument ("t").type_float(0.0, 1.0)

  + Option ("maxiter",
            "the maximum number of iterations of the conjugate gradient solver. (default = " + str(DEFAULT_MAXITER) + ")")
    + Argument ("n").type_integer(1);

}


typedef float value_type;



void run ()
{
  auto dwi = Image<value_type>::open(argument[0]);

  // Read parameters
  int lmax = get_option_value("lmax", DEFAULT_LMAX);
  value_type tol = get_option_value("tolerance", DEFAULT_TOL);
  size_t maxiter = get_option_value("maxiter", DEFAULT_MAXITER);

  // Read motion parameters
  auto opt = get_options("motion");
  Eigen::MatrixXf motion;
  if (opt.size())
    motion = load_matrix<float>(opt[0][0]);


  // Force single-shell until multi-shell basis is implemented
  auto grad = DWI::get_valid_DW_scheme (dwi);
  DWI::Shells shells (grad);
  shells.select_shells (true, false, true);
  auto idx = shells.largest().get_volumes();

  // Select subset
  auto dwisub = Adapter::make <Adapter::Extract1D> (dwi, 3, container_cast<vector<int>> (idx));
  Eigen::MatrixXf gradsub (idx.size(), grad.cols());
  for (size_t i = 0; i < idx.size(); i++)
    gradsub.row(i) = grad.row(idx[i]).template cast<float>();


  // Check dimensions
  if (motion.size() && motion.cols() != 6)
    throw Exception("No. columns in motion parameters must equal 6.");
  if (motion.size() && (motion.rows() != dwisub.size(3)) && (motion.rows() != dwisub.size(3) * dwisub.size(2)))
    throw Exception("No. rows in motion parameters must equal the number of DWI volumes or slices.");


  // Set up scattered data matrix
  INFO("initialise reconstruction matrix");
  DWI::ReconMatrix R (dwisub, motion, gradsub, lmax);

  // Read input data to vector
  Eigen::VectorXf y (dwisub.size(0)*dwisub.size(1)*dwisub.size(2)*dwisub.size(3));
  size_t j = 0;
  for (auto l = Loop("loading image data", {0, 1, 2, 3})(dwisub); l; l++, j++)
    y[j] = dwisub.value();


  // Fit scattered data in basis...
  INFO("solve with conjugate gradient method");

  Eigen::initParallel();
  Eigen::setNbThreads(Thread::number_of_threads());
  VAR(Thread::number_of_threads());
  VAR(Eigen::nbThreads());

  Eigen::LeastSquaresConjugateGradient<DWI::ReconMatrix, Eigen::IdentityPreconditioner> lscg;
  lscg.compute(R);

  lscg.setTolerance(tol);
  lscg.setMaxIterations(maxiter);

  Eigen::VectorXf x = lscg.solve(y);

  std::cout << "LSCG: #iterations: " << lscg.iterations() << ", estimated error: " << lscg.error() << std::endl;


  // Write result to output file
  Header header (dwisub);
  DWI::stash_DW_scheme (header, gradsub);
  header.size(3) = Math::SH::NforL(lmax);
  auto out = Image<value_type>::create (argument[1], header);

  j = 0;
  for (auto l = Loop("writing result to image", {0, 1, 2, 3})(out); l; l++, j++) {
    out.value() = x[j];
  }


}




