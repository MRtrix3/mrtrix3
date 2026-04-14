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
#include "image.h"
#include "mrtrix.h"

#include "fixel/validate.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Validate a so-called peaks image (a set of 3-vectors per voxel where each encodes an orientation)";

  DESCRIPTION
  + "This command checks that an image conforms to the requirements of a"
    " peaks image, in which successive triplets of volumes encode the"
    " (x, y, z) Cartesian components of one peak direction per triplet."

  + "The following checks are performed:"

  + "1. The image must be of floating-point type."

  + "2. The image must be 4-dimensional,"
    " with the number of volumes a multiple of three."

  + "3. Where a voxel contains fewer peaks than the maximum,"
    " the unfilled peak slots must use a single consistent fill convention"
    " across the entire image:"
    " either all three components of the fill triplet are zero,"
    " or all three components are NaN."
    " Both conventions must not be mixed within the same image."

  + "4. When the fill convention is NaN,"
    " every individual triplet must be either entirely finite"
    " or entirely NaN."
    " Triplets that are partly NaN and partly non-NaN are not valid."

  + "5. Every non-fill peak must contain finite values."

  + "The command also reports the range of norms across all non-fill peaks,"
    " indicating whether the image stores unit-norm directions only"
    " or whether a quantitative value is associated with each peak"
    " (encoded in the vector norm).";

  ARGUMENTS
  + Argument ("image", "the input peaks image").type_image_in();
}
// clang-format on

void run() {
  Header H = Header::open(argument[0]);
  // validate_header() throws on any structural violation.
  Peaks::validate_header(H);

  // validate_image() throws on any content violation.
  auto image = H.get_image<float>();
  const Peaks::PeaksValidation result = Peaks::validate_image(image);

  const ssize_t n_peaks = H.size(3) / 3;
  CONSOLE("Image \"" + H.name() + "\": " + str(n_peaks) + " peak slot(s) per voxel");

  // Report fill-value convention.
  if (!result.fill_value.has_value()) {
    CONSOLE("Fill value: none detected (all voxels fully populated)");
  } else {
    CONSOLE(std::string("Fill value: ") + (std::isnan(*result.fill_value) ? "NaN" : "zero"));
  }

  // Report norm range and interpretation.
  if (!std::isfinite(result.norm_min)) {
    WARN("No non-fill peaks found in the image");
    return;
  }

  constexpr float unit_tol = 1e-4F;
  const bool unit_norm = std::fabs(result.norm_min - 1.0F) <= unit_tol && //
                         std::fabs(result.norm_max - 1.0F) <= unit_tol;   //

  if (unit_norm) {
    CONSOLE("Peak norms: all peaks are unit-norm (directions image; no quantitative amplitude)");
  } else {
    CONSOLE("Peak norms: range [" + str(result.norm_min) + ", " + str(result.norm_max) + "]" + //
            " (quantitative amplitudes encoded in vector norm)");                              //
  }
}
