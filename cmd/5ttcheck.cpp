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


#include "app.h"
#include "command.h"
#include "datatype.h"
#include "image.h"
#include "image_helpers.h"

#include "algo/copy.h"
#include "algo/loop.h"

#include "dwi/tractography/ACT/act.h"


using namespace MR;
using namespace App;



#define MAX_ERROR 0.001



void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "thoroughly check that one or more images conform to the expected ACT five-tissue-type (5TT) format";

  ARGUMENTS
  + Argument ("input", "the 5TT image(s) to be tested").type_image_in().allow_multiple();

  OPTIONS
  + Option ("masks", "output mask images highlighting voxels where the input does not conform to 5TT requirements")
    + Argument ("prefix").type_text();
}



void run ()
{
  const std::string mask_prefix = get_option_value<std::string> ("masks", "");

  size_t error_count = 0;
  for (size_t i = 0; i != argument.size(); ++i) {

    auto in = Image<float>::open (argument[i]);

    Image<bool> voxels;
    Header H_out (in);
    H_out.ndim() = 3;
    H_out.datatype() = DataType::Bit;
    if (mask_prefix.size())
      voxels = Image<bool>::scratch (H_out, "Scratch image for " + argument[i]);

    try {

      // This checks:
      //   - Floating-point image
      //   - 4-dimensional
      //   - 5 volumes
      DWI::Tractography::ACT::verify_5TT_image (in);

      // Manually loop through all voxels, make sure that for every voxel the
      //   sum of tissue partial volumes is either 0 or 1 (or close enough)
      size_t voxel_error_sum = 0;
      for (auto outer = Loop(in, 0, 3) (in); outer; ++outer) {
        default_type sum = 0.0;
        for (auto inner = Loop(3) (in); inner; ++inner)
          sum += in.value();
        if (!sum) continue;
        if (std::abs (sum-1.0) > MAX_ERROR) {
          ++voxel_error_sum;
          if (voxels.valid()) {
            assign_pos_of (in, 0, 3).to (voxels);
            voxels.value() = true;
          }
        }
      }

      if (voxel_error_sum) {
        WARN ("Image \"" + argument[i] + "\" contains " + str(voxel_error_sum) + " brain voxels with non-unity sum of partial volume fractions");
        ++error_count;
        if (voxels.valid()) {
          auto out = Image<bool>::create (mask_prefix + Path::basename (argument[i]), H_out);
          copy (voxels, out);
        }
      } else {
        INFO ("Image \"" + argument[i] + "\" conforms to 5TT format");
      }

    } catch (...) {
      WARN ("Image \"" + argument[i] + "\" does not conform to fundamental 5TT format requirements");
      ++error_count;
    }
  }

  if (error_count) {
    if (argument.size() > 1)
      throw Exception (str(error_count) + " input image" + (error_count > 1 ? "s do" : " does") + " not conform to 5TT format");
    else
      throw Exception ("Input image does not conform to 5TT format");
  }
}

