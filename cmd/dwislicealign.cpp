/* Copyright (c) 2017-2019 Daan Christiaens
 *
 * MRtrix and this add-on module are distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#include "command.h"
#include "dwi/gradient.h"
#include "file/matrix.h"
#include "image.h"
#include "thread_queue.h"

#include "dwi/svr/psf.h"
#include "dwi/svr/register.h"

constexpr float DEFAULT_SSPW = 1.0F;

using namespace MR;
using namespace App;

// clang-format off
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

  + Option ("ssp", "SSP vector or slice thickness in voxel units (default = 1).")
    + Argument ("w").type_text()

  + Option ("init", "motion initialisation")
    + Argument ("motion").type_file_in()

  + Option ("maxiter", "maximum no. iterations for the registration")
    + Argument ("n").type_integer(0)

  + DWI::GradImportOptions();

}
// clang-format on

using value_type = float;

void run() {
  // input data
  auto data = Image<value_type>::open(argument[0]);
  auto grad = DWI::get_DW_scheme(data);

  // input template
  auto mssh = Image<value_type>::open(argument[1]);
  if (mssh.ndim() != 5)
    throw Exception("5-D MSSH image expected.");

  // index shells
  auto bvals = parse_floats(mssh.keyval().find("shells")->second);

  // mask
  auto mask = Image<bool>();
  auto opt = get_options("mask");
  if (!opt.empty()) {
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
      throw Exception("multiband factor invalid.");
  }

  // SSP
  DWI::SVR::SSP<float> ssp(DEFAULT_SSPW);
  opt = get_options("ssp");
  if (!opt.empty()) {
    std::string const t = opt[0][0];
    try {
      ssp = DWI::SVR::SSP<float>(std::stof(t));
    } catch (std::invalid_argument &e) {
      try {
        Eigen::VectorXf const v = File::Matrix::load_vector<float>(t);
        ssp = DWI::SVR::SSP<float>(v);
      } catch (...) {
        throw Exception("Invalid argument for SSP.");
      }
    }
  }

  // settings and initialisation
  size_t const niter = get_option_value("maxiter", 0);
  Eigen::MatrixXf init(data.size(3), 6);
  init.setZero();
  opt = get_options("init");
  if (!opt.empty()) {
    init = File::Matrix::load_matrix<float>(opt[0][0]);
    if ((init.cols() != 6) || (((data.size(3) * data.size(2)) % init.rows()) != 0))
      throw Exception("dimension mismatch in motion initialisaton.");
  }

  // run registration
  DWI::SVR::SliceAlignSource source(data.size(3), data.size(2), mb, grad, bvals, init);
  DWI::SVR::SliceAlignPipe pipe(data, mssh, mask, mb, niter, ssp);
  DWI::SVR::SliceAlignSink sink(data.size(3), data.size(2), mb);
  Thread::run_queue(source, DWI::SVR::SliceIdx(), Thread::multi(pipe), DWI::SVR::SliceIdx(), sink);

  // output
  File::Matrix::save_matrix(sink.get_motion(), argument[2]);
}
