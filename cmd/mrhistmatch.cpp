/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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
#include <cmath>

#include "command.h"
#include "datatype.h"
#include "header.h"
#include "image.h"

#include "algo/histogram.h"
#include "algo/loop.h"


using namespace MR;
using namespace App;

void usage () {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au";

  SYNOPSIS = "Modify the intensities of one image to match the histogram of another via a non-linear transform";

  ARGUMENTS
    + Argument ("input", "the input image to be modified").type_image_in ()
    + Argument ("target", "the input image from which to derive the target histogram").type_image_in()
    + Argument ("output", "the output image").type_image_out();

  OPTIONS
    + Option ("mask_input", "only generate input histogram based on a specified binary mask image")
      + Argument ("image").type_image_in ()
    + Option ("mask_target", "only generate target histogram based on a specified binary mask image")
      + Argument ("image").type_image_in ()

    // TODO Remove before release
    + Option ("cdfs", "output the histogram CDFs to a text file (for debugging).")
      + Argument ("path").type_file_out ()

    + Option ("bins", "the number of bins to use to generate the histograms")
      + Argument ("num").type_integer (2);

  REFERENCES
    + "* If using inverse contrast normalization for inter-modal (DWI - T1) registration:\n"
      "Bhushan, C.; Haldar, J. P.; Choi, S.; Joshi, A. A.; Shattuck, D. W. & Leahy, R. M. "
      "Co-registration and distortion correction of diffusion and anatomical images based on inverse contrast normalization. "
      "NeuroImage, 2015, 115, 269-280";

}



using value_type = default_type;



void run ()
{

  auto input  = Image<float>::open (argument[0]);
  auto target = Image<float>::open (argument[1]);

  if (input.ndim() > 3 || target.ndim() > 3)
    throw Exception ("mrhistmatch currently only works on 3D images");

  const size_t nbins = get_option_value ("bins", 0);

  Image<bool> mask_input, mask_target;

  auto opt = get_options ("mask_input");
  if (opt.size()) {
    mask_input = Image<bool>::open (opt[0][0]);
    check_dimensions (input, mask_input);
  }
  opt = get_options ("mask_target");
  if (opt.size()) {
    mask_target = Image<bool>::open (opt[0][0]);
    check_dimensions (target, mask_target);
  }

  Algo::Histogram::Calibrator calib_input (nbins, true);
  Algo::Histogram::calibrate (calib_input, input, mask_input);
  INFO ("Input histogram ranges from " + str(calib_input.get_min()) + " to " + str(calib_input.get_max()) + "; using " + str(calib_input.get_num_bins()) + " bins");
  Algo::Histogram::Data hist_input = Algo::Histogram::generate (calib_input, input, mask_input);

  Algo::Histogram::Calibrator calib_target (nbins, true);
  Algo::Histogram::calibrate (calib_target, target, mask_target);
  INFO ("Target histogram ranges from " + str(calib_target.get_min()) + " to " + str(calib_target.get_max()) + "; using " + str(calib_target.get_num_bins()) + " bins");
  Algo::Histogram::Data hist_target = Algo::Histogram::generate (calib_target, target, mask_target);

  // Non-linear intensity mapping determined within this class
  Algo::Histogram::Matcher matcher (hist_input, hist_target);

  // Generate the output image
  Header H (input);
  H.datatype() = DataType::Float32;
  H.datatype().set_byte_order_native();
  auto output = Image<float>::create (argument[2], H);
  for (auto l = Loop(input) (input, output); l; ++l) {
    if (std::isfinite(static_cast<float>(input.value()))) {
      output.value() = matcher (input.value());
    } else {
      output.value() = 0.0;
    }
  }

}

