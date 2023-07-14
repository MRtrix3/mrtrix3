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

#include "command.h"
#include "image.h"
#include "phase_encoding.h"
#include "progressbar.h"
#include "algo/threaded_copy.h"
#include "math/least_squares.h"
#include "dwi/gradient.h"


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Convert mean dwi (trace-weighted) images to mean ADC maps";

  ARGUMENTS
    + Argument ("input", "the input image.").type_image_in ()
    + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
    + DWI::GradImportOptions();
}



using value_type = float;



class DWI2ADC {
  public:
    DWI2ADC (const Eigen::MatrixXd& binv, size_t dwi_axis) :
      dwi (binv.cols()),
      adc (2),
      binv (binv),
      dwi_axis (dwi_axis) { }
    DWI2ADC (const DWI2ADC& that) :
      dwi (that.dwi.size()),
      adc (that.adc.size()),
      binv (that.binv),
      dwi_axis (that.dwi_axis) {}

    template <class DWIType, class ADCType>
      void operator() (DWIType& dwi_image, ADCType& adc_image) {
        for (auto l = Loop (dwi_axis) (dwi_image); l; ++l) {
          value_type val = dwi_image.value();
          dwi[dwi_image.index (dwi_axis)] = val ? std::log (val) : 1.0e-12;
        }

        adc = binv * dwi;

        adc_image.index(3) = 0;
        adc_image.value() = std::exp (adc[0]);
        adc_image.index(3) = 1;
        adc_image.value() = adc[1];
      }

  protected:
    Eigen::VectorXd dwi, adc;
    const Eigen::MatrixXd& binv;
    const size_t dwi_axis;
};




void run () {
  auto dwi = Header::open (argument[0]).get_image<value_type>();
  auto grad = DWI::get_DW_scheme (dwi);

  size_t dwi_axis = 3;
  while (dwi.size (dwi_axis) < 2)
    ++dwi_axis;
  INFO ("assuming DW images are stored along axis " + str (dwi_axis));

  Eigen::MatrixXd b (grad.rows(), 2);
  for (ssize_t i = 0; i < b.rows(); ++i) {
    b(i,0) = 1.0;
    b(i,1) = -grad (i,3);
  }

  auto binv = Math::pinv (b);

  Header header (dwi);
  header.datatype() = DataType::Float32;
  DWI::stash_DW_scheme (header, grad);
  PhaseEncoding::clear_scheme (header);
  header.ndim() = 4;
  header.size(3) = 2;

  auto adc = Image<value_type>::create (argument[1], header);

  ThreadedLoop ("computing ADC values", dwi, 0, 3)
    .run (DWI2ADC (binv, dwi_axis), dwi, adc);
}


