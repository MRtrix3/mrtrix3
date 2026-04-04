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

#include "registration/warp/validate.h"

#include <cmath>
#include <limits>
#include <string>

#include "algo/loop.h"
#include "app.h"
#include "exception.h"
#include "header.h"
#include "image.h"
#include "mrtrix.h"

namespace MR::Registration::Warp {

WarpValidation validate_warp(const Header &H) {
  WarpValidation result;

  // ---------------------------------------------------------------
  // Check 1: the image must be of floating-point type.
  // ---------------------------------------------------------------
  if (!H.datatype().is_floating_point())
    throw Exception("Warp image \"" + H.name() +
                    "\":"
                    " expected a floating-point data type"
                    " (got " +
                    H.datatype().description() + ")");

  // ---------------------------------------------------------------
  // Check 2: structural dimensions.
  //   - 4D with size(3) == 3: simple warp (displacement or deformation field)
  //   - 5D with size(3) == 3 and size(4) == 4: full warp field
  //   - anything else: hard error
  // ---------------------------------------------------------------
  if (H.ndim() == 4) {
    if (H.size(3) != 3)
      throw Exception("Warp image \"" + H.name() +
                      "\":"
                      " a 4D warp image (displacement or deformation field)"
                      " must have exactly 3 volumes in the 4th dimension"
                      " (found " +
                      str(H.size(3)) + ")");
    result.format = WarpFormat::Simple;

  } else if (H.ndim() == 5) {
    if (H.size(3) != 3)
      throw Exception("Warp image \"" + H.name() +
                      "\":"
                      " a 5D full warp image must have exactly 3 volumes"
                      " in the 4th dimension (the x/y/z components)"
                      " (found " +
                      str(H.size(3)) + ")");
    if (H.size(4) != 4)
      throw Exception("Warp image \"" + H.name() +
                      "\":"
                      " a 5D full warp image must have exactly 4 volume groups"
                      " in the 5th dimension"
                      " (found " +
                      str(H.size(4)) + ")");
    result.format = WarpFormat::Full;

  } else {
    throw Exception("Warp image \"" + H.name() +
                    "\":"
                    " expected a 4D image (displacement or deformation field)"
                    " or a 5D image (full warp field)"
                    " (found " +
                    str(H.ndim()) + " dimensions)");
  }

  // ---------------------------------------------------------------
  // Check 3: for full warp images, the header must contain the
  // "linear1" and "linear2" linear transform keys.
  // ---------------------------------------------------------------
  if (result.format == WarpFormat::Full) {
    if (H.keyval().find("linear1") == H.keyval().end())
      throw Exception("Warp image \"" + H.name() +
                      "\":"
                      " full warp image header is missing the \"linear1\""
                      " linear transform field");
    if (H.keyval().find("linear2") == H.keyval().end())
      throw Exception("Warp image \"" + H.name() +
                      "\":"
                      " full warp image header is missing the \"linear2\""
                      " linear transform field");
  }

  // ---------------------------------------------------------------
  // Checks 4 & 5: scan every warp triplet.
  //
  // For a simple (4D) warp there is one triplet per spatial voxel,
  // formed by dim-3 indices 0, 1, 2.
  //
  // For a full (5D) warp there are four triplets per spatial voxel
  // (one per volume group in dim 4), each formed by dim-3 indices 0, 1, 2.
  //
  // A triplet may be a fill value (all-zero or all-NaN) or a data value
  // (all-finite).  The fill convention must be consistent throughout:
  //   fill_kind == 0  →  not yet determined
  //   fill_kind == 1  →  zero fill
  //   fill_kind == 2  →  NaN fill
  //
  // If the fill convention is NaN, any partially-NaN triplet is invalid.
  // ---------------------------------------------------------------
  auto img = Image<float>::open(H.name());

  const bool is_full = (result.format == WarpFormat::Full);
  const ssize_t n_groups = is_full ? 4 : 1;

  int fill_kind = 0;

  for (auto l = Loop("Validating warp image", img, 0, 3)(img); l; ++l) {
    for (ssize_t g = 0; g < n_groups; ++g) {
      if (is_full)
        img.index(4) = g;

      img.index(3) = 0;
      const float x = img.value();
      img.index(3) = 1;
      const float y = img.value();
      img.index(3) = 2;
      const float z = img.value();

      const bool x_nan = std::isnan(x);
      const bool y_nan = std::isnan(y);
      const bool z_nan = std::isnan(z);
      const bool any_nan = x_nan || y_nan || z_nan;
      const bool all_nan = x_nan && y_nan && z_nan;
      const bool all_zero = (x == 0.0F) && (y == 0.0F) && (z == 0.0F);

      // ---------------------------------------------------------------
      // Check 4: fill-value consistency.
      // Triplets that are all-zero or all-NaN are treated as fill.
      // The same fill convention must be used throughout the image.
      // ---------------------------------------------------------------
      if (all_zero || all_nan) {
        const int this_kind = all_nan ? 2 : 1;
        if (fill_kind == 0) {
          fill_kind = this_kind;
          result.fill_value = all_nan ? std::numeric_limits<float>::quiet_NaN() : 0.0F;
          result.fill_value_seen = true;
        } else if (fill_kind != this_kind) {
          throw Exception("Warp image \"" + H.name() +
                          "\":"
                          " mixed fill values detected"
                          " (both zero and NaN triplets are present);"
                          " a single fill convention must be used throughout");
        }
        continue;
      }

      // ---------------------------------------------------------------
      // Check 5: NaN-fill coherence.
      // A triplet that is not all-NaN and not all-zero must be entirely
      // finite.  Any partially-NaN triplet is invalid regardless of
      // which fill convention the image uses.
      // ---------------------------------------------------------------
      if (any_nan) {
        // We already know the triplet is not all-NaN (handled above),
        // so this is a partial-NaN triplet — always invalid.
        const std::string loc = "[" + str(img.index(0)) + "," + str(img.index(1)) + "," + str(img.index(2)) + "]";
        const std::string grp = is_full ? (", group " + str(g)) : "";
        throw Exception("Warp image \"" + H.name() +
                        "\":"
                        " voxel " +
                        loc + grp +
                        " contains a mix of NaN and non-NaN values"
                        " (a warp triplet must be either all-finite or all-NaN)");
      }
    }
  }

  return result;
}

void debug_validate_warp(const Header &H) {
  if (App::log_level < 3)
    return;
  try {
    const WarpValidation v = validate_warp(H);
    const std::string fmt = v.format == WarpFormat::Simple ? "simple (displacement or deformation field)" : "full warp";
    DEBUG("Warp image \"" + H.name() + "\": " + fmt + " format");
    if (!v.fill_value_seen)
      DEBUG("Warp image \"" + H.name() + "\": no fill triplets detected");
    else if (std::isnan(v.fill_value))
      DEBUG("Warp image \"" + H.name() + "\": fill value is NaN");
    else
      DEBUG("Warp image \"" + H.name() + "\": fill value is zero");
  } catch (const Exception &e) {
    DEBUG("Warp image \"" + H.name() + "\": validation failed: " + e[0]);
  }
}

} // namespace MR::Registration::Warp
