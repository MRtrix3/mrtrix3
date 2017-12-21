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
#include "thread_queue.h"
#include "dwi/gradient.h"

#include "dwi/svr/register.h"


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk)";

  SYNOPSIS = "Register multi-shell spherical harmonics image to DWI slices or volumes.";

  DESCRIPTION
  + "This command takes DWI data and a multi-shell spherical harmonics (MSSH) signal "
    "prediction to estimate subject motion parameters with volume-to-slice registration.";

  ARGUMENTS
  + Argument ("data", "the input DWI data.").type_image_in()

  + Argument ("mssh", "the input MSSH prediction.").type_image_in()

  + Argument ("out", "the output motion parameters.").type_file_out();

  OPTIONS
  + Option ("mask", "image mask")
    + Argument ("m").type_image_in()

  + Option ("mb", "multiband factor. (default = 0; v2v registration)")
    + Argument ("factor").type_integer(0)

  + Option ("init", "motion initialisation")
    + Argument ("motion").type_file_in()

  + Option ("maxiter", "maximum no. iterations for the registration")
    + Argument ("n").type_integer(0)

  + DWI::GradImportOptions();

}


using value_type = float;


inline vector<size_t> get_bidx(const Eigen::MatrixXd& grad, const vector<double> bvals)
{
  vector<size_t> bidx;
  for (int i = 0; i < grad.rows(); i++) {
    size_t j = 0;
    for (auto b : bvals) {
      if (grad(i,3) > (b - DWI_SHELLS_EPSILON) && grad(i,3) < (b + DWI_SHELLS_EPSILON)) {
        bidx.push_back(j); break;
      } else j++;
    }
    if (j == bvals.size())
      throw Exception("invalid bvalues in gradient table.");
  }
  return bidx;
}


void run ()
{
  // input data
  auto data = Image<value_type>::open(argument[0]);
  auto grad = DWI::get_valid_DW_scheme (data);

  // input template
  auto mssh = Image<value_type>::open(argument[1]);
  if (mssh.ndim() != 5)
    throw Exception("5-D MSSH image expected.");

  // index shells
  auto bvals = parse_floats(mssh.keyval().find("shells")->second);
  auto bidx = get_bidx(grad, bvals);

  // mask
  auto mask = Image<bool>();
  auto opt = get_options("mask");
  if (opt.size()) {
    mask = Image<bool>::open(opt[0][0]);
    check_dimensions(data, mask, 0, 3);
  }

  // multiband factor
  size_t mb = get_option_value("mb", 0);
  if (mb == 0 || mb == data.size(2)) {
    mb = data.size(2);
    INFO("volume-to-volume registration.");
  } else {
    if (data.size(2) % mb != 0)
      throw Exception ("multiband factor invalid.");
  }

  // settings and initialisation
  size_t niter = get_option_value("maxiter", 0);
  Eigen::MatrixXf init (data.size(2), 6); init.setZero();
  opt = get_options("init");
  if (opt.size()) {
    init = load_matrix<float>(opt[0][0]);
    if (init.cols() != 6 || ((data.size(3)*data.size(2)) % init.rows()))
      throw Exception("dimension mismatch in motion initialisaton.");
  }

  // run registration
  DWI::SVR::SliceAlignSource source (data.size(3), data.size(2), mb, grad, bvals, init);
  DWI::SVR::SliceAlignSink sink (data.size(3), data.size(2), mb);
  Thread::run_queue(source, DWI::SVR::SliceIdx(), sink);

  // output
  save_matrix(sink.get_motion(), argument[2]);

}



