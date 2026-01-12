/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "algo/threaded_copy.h"
#include "command.h"
#include "dwi/gradient.h"
#include "image.h"
#include "math/least_squares.h"
#include "metadata/phase_encoding.h"
#include "progressbar.h"

using namespace MR;
using namespace App;

using value_type = float;

constexpr value_type ivim_cutoff_default = 120.0F;

// clang-format off
void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com) and Daan Christiaens (daan.christiaens@kuleuven.be)";

  SYNOPSIS = "Calculate ADC and/or IVIM parameters.";

  DESCRIPTION
  + "The command estimates the Apparent Diffusion Coefficient (ADC) "
    "using the isotropic mono-exponential model: S(b) = S(0) * exp(-ADC * b). "
    "The value of S(0) can be optionally exported using command-line option -szero."

  + "When using the -ivim option, the command will additionally estimate the "
    "Intra-Voxel Incoherent Motion (IVIM) parameters f and D', i.e., the perfusion "
    "fraction and the pseudo-diffusion coefficient. IVIM assumes a bi-exponential "
    "model: S(b) = S(0) * ((1-f) * exp(-D * b) + f * exp(-D' * b)). This command "
    "adopts a 2-stage fitting strategy, in which the ADC is first estimated based on "
    "the DWI data with b > cutoff, and the other parameters are estimated subsequently. "
    "The output consists of 4 volumes, respectively S(0), D, f, and D'."

  + "Note that this command ignores the gradient orientation entirely. "
    "If a conventional DWI series is provided as input, "
    "all volumes will contribute equally toward the model fit "
    "irrespective of direction of diffusion sensitisation; "
    "DWI data should therefore ideally consist of isotropically-distributed gradient directions. "
    "The approach can alternatively be applied to mean DWI (trace-weighted) images.";

  ARGUMENTS
    + Argument ("input", "the input image").type_image_in()
    + Argument ("output", "the output ADC image").type_image_out();

  OPTIONS
    + Option("szero",
             "export image of S(0); "
             "that is, the model-estimated signal intensity in the absence of diffusion weighting")
      + Argument("image").type_image_out()

    + Option ("ivim", "also estimate IVIM parameters in 2-stage fit, "
                      "yielding two images encoding signal fraction and diffusivity respectively of perfusion1 component")
      + Argument("fraction").type_image_out()
      + Argument("diffusivity").type_image_out()

    + Option ("cutoff", "minimum b-value for ADC estimation in IVIM fit "
                        "(default = " + str(ivim_cutoff_default) + " s/mm^2).")
    +   Argument ("bval").type_float (0.0, 1000.0)

    + DWI::GradImportOptions();

  REFERENCES
  + "Le Bihan, D.; Breton, E.; Lallemand, D.; Aubin, M.L.; Vignaud, J.; Laval-Jeantet, M. "
    "Separation of diffusion and perfusion in intravoxel incoherent motion MR imaging. "
    "Radiology, 1988, 168, 497-505."

  + "If using -ivim option: "
    "Jalnefjord, O.; Andersson, M.; Montelius; M.; Starck, G.; Elf, A.; Johanson, V.; Svensson, J.; Ljungberg, M. "
    "Comparison of methods for estimation of the intravoxel incoherent motion (IVIM) "
    "diffusion coefficient (D) and perfusion fraction (f). "
    "Magn Reson Mater Phy, 2018, 31, 715-723.";
}
// clang-format on

class DWI2ADC {
public:
  DWI2ADC(const Header &header, const Eigen::VectorXd &bvals, const size_t dwi_axis)
      : H(header),
        bvals(bvals),
        dwi(bvals.size()),
        b(bvals.size(), 2),
        logszero_and_adc(2),
        dwi_axis(dwi_axis),
        ivim_cutoff(std::numeric_limits<value_type>::signaling_NaN()) {
    for (ssize_t i = 0; i < b.rows(); ++i) {
      b(i, 0) = 1.0;
      b(i, 1) = -bvals(i);
    }
    binv = Math::pinv(b);
  }

  void set_bzero_path(std::string_view path) { szero_image = Image<value_type>::create(path, H); }

  void initialise_ivim(std::string_view ivimfrac_path, std::string_view ivimdiff_path, const value_type cutoff) {
    ivimfrac_image = Image<value_type>::create(ivimfrac_path, H);
    ivimdiff_image = Image<value_type>::create(ivimdiff_path, H);
    ivim_cutoff = cutoff;
    // select volumes with b-value > cutoff
    for (ssize_t j = 0; j < bvals.size(); j++) {
      if (bvals[j] > cutoff)
        ivim_suprathresh_idx.push_back(j);
    }
    const Eigen::MatrixXd bsub = b(ivim_suprathresh_idx, Eigen::all);
    bsubinv = Math::pinv(bsub);
  }

  void operator()(Image<value_type> &dwi_image, Image<value_type> &adc_image) {
    for (auto l = Loop(dwi_axis)(dwi_image); l; ++l) {
      const value_type val = dwi_image.value();
      dwi[dwi_image.index(dwi_axis)] = val > 1.0e-12 ? std::log(val) : 1.0e-12;
    }

    if (std::isnan(ivim_cutoff)) {
      logszero_and_adc = binv * dwi;
    } else {
      dwisub = dwi(ivim_suprathresh_idx);
      logszero_and_adc = bsubinv * dwisub;
    }

    adc_image.value() = static_cast<value_type>(logszero_and_adc[1]);

    if (std::isnan(ivim_cutoff)) {
      if (szero_image.valid()) {
        assign_pos_of(adc_image).to(szero_image);
        szero_image.value() = static_cast<value_type>(std::exp(logszero_and_adc[0]));
      }
      return;
    }

    const double A = std::exp(logszero_and_adc[0]);
    const double D = logszero_and_adc[1];
    const Eigen::VectorXd logS = logszero_and_adc[0] - D * bvals.array();
    Eigen::VectorXd logdiff = (dwi.array() > logS.array()).select(dwi, logS);
    logdiff.array() += Eigen::log(1 - Eigen::exp(-(dwi - logS).array().abs()));
    logszero_and_adc = binv * logdiff;
    const double C = std::exp(logszero_and_adc[0]);
    const double Dstar = logszero_and_adc[1];
    const double S0 = A + C;
    const double f = C / S0;
    if (szero_image.valid()) {
      assign_pos_of(adc_image).to(szero_image);
      szero_image.value() = static_cast<value_type>(S0);
    }
    assign_pos_of(adc_image).to(ivimfrac_image, ivimdiff_image);
    ivimfrac_image.value() = static_cast<value_type>(f);
    ivimdiff_image.value() = static_cast<value_type>(Dstar);
  }

private:
  const Header H;
  Eigen::VectorXd bvals, dwi, dwisub, logszero_and_adc;
  Eigen::MatrixXd b, binv;
  const size_t dwi_axis;

  // Optional export
  Image<value_type> szero_image;

  // Members relating to IVIM operation
  std::vector<size_t> ivim_suprathresh_idx;
  Eigen::MatrixXd bsubinv;
  Image<value_type> ivimfrac_image;
  Image<value_type> ivimdiff_image;
  value_type ivim_cutoff;
};

void run() {
  auto H_in = Header::open(argument[0]);
  auto grad = DWI::get_DW_scheme(H_in);
  size_t dwi_axis = 3;
  while (H_in.size(dwi_axis) < 2)
    ++dwi_axis;
  INFO("assuming DW images are stored along axis " + str(dwi_axis));

  Header H_out(H_in);
  H_out.datatype() = DataType::Float32;
  H_out.datatype().set_byte_order_native();
  H_out.ndim() = 3;
  DWI::stash_DW_scheme(H_out, grad);
  Metadata::PhaseEncoding::clear_scheme(H_out.keyval());

  DWI2ADC functor(H_out, grad.col(3), dwi_axis);

  auto opt = get_options("szero");
  if (!opt.empty())
    functor.set_bzero_path(opt[0][0]);
  opt = get_options("ivim");
  if (!opt.empty())
    functor.initialise_ivim(opt[0][0], opt[0][1], get_option_value("cutoff", ivim_cutoff_default));

  auto dwi = H_in.get_image<value_type>();
  auto adc = Image<value_type>::create(argument[1], H_out);

  ThreadedLoop("computing ADC values", H_out).run(functor, dwi, adc);
}
