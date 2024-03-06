/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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
#include "phase_encoding.h"
#include "progressbar.h"

using namespace MR;
using namespace App;

// clang-format off
void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com) and Daan Christiaens (daan.christiaens@kuleuven.be)";

  SYNOPSIS = "Calculate ADC and/or IVIM parameters.";

  DESCRIPTION
  + "By default, the command will estimate the Apparent Diffusion Coefficient (ADC) "
    "using the isotropic mono-exponential model: S(b) = S(0) * exp(-D * b). "
    "The output consists of 2 volumes, respectively S(0) and D."

  + "When using the -ivim option, the command will additionally estimate the "
    "Intra-Voxel Incoherent Motion (IVIM) parameters f and D', i.e., the perfusion "
    "fraction and the pseudo-diffusion coefficient. IVIM assumes a bi-exponential "
    "model: S(b) = S(0) * ((1-f) * exp(-D * b) + f * exp(-D' * b)). This command "
    "adopts a 2-stage fitting strategy, in which the ADC is first estimated based on "
    "the DWI data with b > cutoff, and the other parameters are estimated subsequently. "
    "The output consists of 4 volumes, respectively S(0), D, f, and D'."
    
  + "Note that this command ignores the gradient orientation entirely. This approach is "
    "therefore only suited for mean DWI (trace-weighted) images or for DWI data collected "
    "with isotropically-distributed gradient directions.";

  ARGUMENTS
    + Argument ("input", "the input image.").type_image_in ()
    + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
    + Option ("ivim", "also estimate IVIM parameters in 2-stage fit.")

    + Option ("cutoff", "minimum b-value for ADC estimation in IVIM fit (default = 120 s/mm^2).")
    +   Argument ("bval").type_integer (0, 1000)

    + DWI::GradImportOptions();
  
  REFERENCES
  + "Le Bihan, D.; Breton, E.; Lallemand, D.; Aubin, M.L.; Vignaud, J.; Laval-Jeantet, M. "
    "Separation of diffusion and perfusion in intravoxel incoherent motion MR imaging. "
    "Radiology, 1988, 168, 497–505."
    
  + "Jalnefjord, O.; Andersson, M.; Montelius; M.; Starck, G.; Elf, A.; Johanson, V.; Svensson, J.; Ljungberg, M. "
    "Comparison of methods for estimation of the intravoxel incoherent motion (IVIM) "
    "diffusion coefficient (D) and perfusion fraction (f). "
    "Magn Reson Mater Phy, 2018, 31, 715–723.";
}
// clang-format on

using value_type = float;

class DWI2ADC {
public:
  DWI2ADC(const Eigen::VectorXd &bvals, size_t dwi_axis, bool ivim, int cutoff)
      : bvals(bvals), dwi(bvals.size()), adc(2), dwi_axis(dwi_axis), ivim(ivim), cutoff(cutoff) {
    Eigen::MatrixXd b(bvals.size(), 2);
    for (ssize_t i = 0; i < b.rows(); ++i) {
      b(i, 0) = 1.0;
      b(i, 1) = -bvals(i);
    }
    binv = Math::pinv(b);
    if (ivim) {
      // select volumes with b-value > cutoff
      for (size_t j = 0; j < bvals.size(); j++) {
        if (bvals[j] > cutoff)
          idx.push_back(j);
      }
      Eigen::MatrixXd bsub = b(idx, Eigen::all);
      bsubinv = Math::pinv(bsub);
    }
  }

  template <class DWIType, class ADCType> void operator()(DWIType &dwi_image, ADCType &adc_image) {
    for (auto l = Loop(dwi_axis)(dwi_image); l; ++l) {
      value_type val = dwi_image.value();
      dwi[dwi_image.index(dwi_axis)] = val > 1.0e-12 ? std::log(val) : 1.0e-12;
    }

    if (ivim) {
      dwisub = dwi(idx);
      adc = bsubinv * dwisub;
    } else {
      adc = binv * dwi;
    }

    adc_image.index(3) = 0;
    adc_image.value() = std::exp(adc[0]);
    adc_image.index(3) = 1;
    adc_image.value() = adc[1];

    if (ivim) {
      double A = std::exp(adc[0]);
      double D = adc[1];
      Eigen::VectorXd logS = adc[0] - D * bvals.array();
      Eigen::VectorXd logdiff = (dwi.array() > logS.array()).select(dwi, logS);
      logdiff.array() += Eigen::log(1 - Eigen::exp(-(dwi - logS).array().abs()));
      adc = binv * logdiff;
      double C = std::exp(adc[0]);
      double Dstar = adc[1];
      double S0 = A + C;
      double f = C / S0;
      adc_image.index(3) = 0;
      adc_image.value() = S0;
      adc_image.index(3) = 2;
      adc_image.value() = f;
      adc_image.index(3) = 3;
      adc_image.value() = Dstar;
    }
  }

protected:
  Eigen::VectorXd bvals, dwi, dwisub, adc;
  Eigen::MatrixXd binv, bsubinv;
  std::vector<size_t> idx;
  const size_t dwi_axis;
  const bool ivim;
  const int cutoff;
};

void run() {
  auto dwi = Header::open(argument[0]).get_image<value_type>();
  auto grad = DWI::get_DW_scheme(dwi);

  size_t dwi_axis = 3;
  while (dwi.size(dwi_axis) < 2)
    ++dwi_axis;
  INFO("assuming DW images are stored along axis " + str(dwi_axis));

  auto opt = get_options("ivim");
  bool ivim = opt.size();
  int bmin = get_option_value("cutoff", 120);

  Header header(dwi);
  header.datatype() = DataType::Float32;
  DWI::stash_DW_scheme(header, grad);
  PhaseEncoding::clear_scheme(header);
  header.ndim() = 4;
  header.size(3) = ivim ? 4 : 2;

  auto adc = Image<value_type>::create(argument[1], header);

  ThreadedLoop("computing ADC values", dwi, 0, 3).run(DWI2ADC(grad.col(3), dwi_axis, ivim, bmin), dwi, adc);
}
