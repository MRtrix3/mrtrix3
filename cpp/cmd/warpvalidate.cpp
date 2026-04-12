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

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

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

  + "5D image with 3 volumes for each of 4 volume groups:"
    " a full warp field as produced by eg. mrregister -nl_warp_full."
    " The image must be 5-dimensional,"
    " with exactly 3 volumes in the 4th dimension (x/y/z components of the warp)"
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
  Header H = Header::open(argument[0]);
  // validate_warp() throws on any structural violation.
  // It is called here in addition to being re-invoked by validate_image() below
  //   so that the datatype of the input image is properly checked
  Registration::Warp::validate_header(H);

  // validate_warp() throws on any content violation.
  auto image = H.get_image<float>();
  const auto result = Registration::Warp::validate_image(image);

  CONSOLE("Warp image \"" + H.name() + "\": valid " + //
          (result.format == WarpFormat::Simple ? "simple (displacement or deformation) field" : "full warp field"));

  CONSOLE("Fill value: " +
          (result.fill_value.has_value() ? str(*result.fill_value) : "not auto-detected from input data"));
}
