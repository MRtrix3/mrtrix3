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

#include "command.h"
#include "header.h"
#include "mrtrix.h"

#include "registration/warp/validate.h"

using namespace MR;
using namespace App;
using namespace MR::Registration::Warp;

// clang-format off
void usage() {

  AUTHOR = "Robert Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Validate a non-linear warp image";

  DESCRIPTION
  + "This command checks that an image conforms to the requirements of a"
    " non-linear warp field."

  + "The format is inferred from the image dimensions:"

  + "4D image with 3 volumes:"
    " a displacement or deformation field."
    " The image must be 4-dimensional with exactly 3 volumes"
    " in the 4th dimension (the x, y, z warp components)."
    " MRtrix3 uses displacement and deformation fields interchangeably;"
    " the structural requirements are identical for both."

  + "5D image with 3 × 4 volumes:"
    " a full warp field as produced by mrregister -nl_warp_full."
    " The image must be 5-dimensional,"
    " with exactly 3 volumes in the 4th dimension (x/y/z components)"
    " and exactly 4 volume groups in the 5th dimension"
    " (image1-to-midway, midway-to-image1, image2-to-midway, midway-to-image2)."
    " The header must also contain \"linear1\" and \"linear2\" fields"
    " encoding the linear transform component for each image."

  + "The following checks are performed:"

  + "1. The image must be of floating-point type."

  + "2. The dimensionality and volume counts must match one of the two"
    " supported formats described above."

  + "3. For full warp images, the header must contain both the \"linear1\""
    " and \"linear2\" linear transform fields."

  + "4. Fill values, used to mark voxels that lie outside the domain of the"
    " warp (e.g. outside a brain mask), must use a single consistent convention"
    " across the entire image: either all fill triplets are all-zero,"
    " or all fill triplets are all-NaN."
    " Both conventions must not be mixed within the same image."

  + "5. Where the fill convention is NaN,"
    " every individual warp triplet must be either entirely finite"
    " or entirely NaN."
    " Triplets that are partly NaN and partly non-NaN are not valid.";

  ARGUMENTS
  + Argument ("image", "the input warp image").type_image_in();
}
// clang-format on

void run() {
  const Header H = Header::open(argument[0]);

  // validate_warp() throws on any structural or content violation.
  const WarpValidation result = validate_warp(H);

  const std::string fmt =
      result.format == WarpFormat::Simple ? "simple (displacement or deformation field)" : "full warp field";

  CONSOLE("Warp image \"" + H.name() + "\": valid " + fmt);

  if (!result.fill_value_seen) {
    CONSOLE("Fill value: none detected (all voxels contain warp data)");
  } else if (std::isnan(result.fill_value)) {
    CONSOLE("Fill value: NaN");
  } else {
    CONSOLE("Fill value: zero");
  }
}
