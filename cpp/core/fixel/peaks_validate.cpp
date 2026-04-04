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

#include "fixel/peaks_validate.h"

#include <cmath>
#include <limits>

#include "algo/loop.h"
#include "app.h"
#include "exception.h"
#include "fixel/helpers.h"
#include "header.h"
#include "image.h"
#include "image_helpers.h"
#include "mrtrix.h"

namespace MR::Peaks {

PeaksValidation validate_peaks_image(const Header &H) {
  // ---------------------------------------------------------------
  // Check 1 & 2: floating-point type, 4D, volumes multiple of 3.
  // Delegate to the existing lightweight structural check.
  // ---------------------------------------------------------------
  Peaks::check(H);

  // Number of peaks slots per voxel.
  const ssize_t n_peaks = H.size(3) / 3;

  // Open a fresh image handle for reading (avoids consuming H's IO handler).
  auto img = Image<float>::open(H.name());

  // Encode fill-value state as a tri-state:
  //   0 = not yet determined
  //   1 = zero fill
  //   2 = NaN fill
  int fill_kind = 0;

  PeaksValidation result;

  for (auto l = Loop("Validating peaks image", img, 0, 3)(img); l; ++l) {
    for (ssize_t p = 0; p < n_peaks; ++p) {
      // Read the three components of peak p.
      img.index(3) = p * 3;
      const float x = img.value();
      img.index(3) = p * 3 + 1;
      const float y = img.value();
      img.index(3) = p * 3 + 2;
      const float z = img.value();

      const bool x_nan = std::isnan(x);
      const bool y_nan = std::isnan(y);
      const bool z_nan = std::isnan(z);
      const bool any_nan = x_nan || y_nan || z_nan;
      const bool all_nan = x_nan && y_nan && z_nan;

      const bool x_zero = (x == 0.0F);
      const bool y_zero = (y == 0.0F);
      const bool z_zero = (z == 0.0F);
      const bool all_zero = x_zero && y_zero && z_zero;

      // ---------------------------------------------------------------
      // Check 3: fill-value consistency.
      // A triplet is a "fill" if it is all-zero or all-NaN.
      // A triplet is a "real peak" if it is fully finite and non-zero.
      // The same fill convention must be used throughout the image.
      // ---------------------------------------------------------------
      if (all_zero || all_nan) {
        // This is a fill triplet.
        const int this_kind = all_nan ? 2 : 1;
        if (fill_kind == 0) {
          // First fill triplet seen — lock in the fill convention.
          fill_kind = this_kind;
          result.fill_value = all_nan ? std::numeric_limits<float>::quiet_NaN() : 0.0F;
          result.fill_value_seen = true;
        } else if (fill_kind != this_kind) {
          throw Exception("Peaks image \"" + H.name() +
                          "\": mixed fill values detected"
                          " (both zero and NaN triplets are present);"
                          " a single fill value must be used throughout");
        }
        continue; // no further checks needed for a valid fill triplet
      }

      // ---------------------------------------------------------------
      // Check 4: NaN-fill coherence.
      // If the fill convention is NaN, then every non-fill triplet must
      // be entirely finite (no individual NaN components are permitted).
      // We also catch partial NaN triplets regardless of fill convention,
      // because a triplet that is partly NaN and partly non-NaN cannot
      // represent either a valid peak or a valid NaN fill.
      // ---------------------------------------------------------------
      if (any_nan) {
        // We already know the triplet is not all-NaN (handled above).
        // So this is a partial-NaN triplet, which is always invalid.
        throw Exception("Peaks image \"" + H.name() + "\": triplet for peak " + str(p) + " at voxel [" +
                        str(img.index(0)) + "," + str(img.index(1)) + "," + str(img.index(2)) +
                        "] contains a mix of NaN and non-NaN values"
                        " (a peak triplet must be either all-finite or all-NaN)");
      }

      // ---------------------------------------------------------------
      // Check 5: non-fill peaks must have a finite, strictly positive norm.
      // ---------------------------------------------------------------
      const float norm = std::sqrt(x * x + y * y + z * z);
      if (!std::isfinite(norm) || norm == 0.0F)
        throw Exception("Peaks image \"" + H.name() + "\": triplet for peak " + str(p) + " at voxel [" +
                        str(img.index(0)) + "," + str(img.index(1)) + "," + str(img.index(2)) +
                        "] has a zero or non-finite norm (" + str(norm) + ")");

      // Track the norm range.
      if (!result.any_peaks_seen) {
        result.norm_min = norm;
        result.norm_max = norm;
        result.any_peaks_seen = true;
      } else {
        if (norm < result.norm_min)
          result.norm_min = norm;
        if (norm > result.norm_max)
          result.norm_max = norm;
      }
    }
  }

  return result;
}

void debug_validate_peaks_image(const Header &H) {
  if (App::log_level < 3)
    return;
  try {
    const PeaksValidation v = validate_peaks_image(H);
    if (v.fill_value_seen)
      DEBUG("Peaks image \"" + H.name() + "\": fill value is " + (std::isnan(v.fill_value) ? "NaN" : "zero"));
    else
      DEBUG("Peaks image \"" + H.name() +
            "\": no fill triplets detected"
            " (all voxels fully populated)");
    if (v.any_peaks_seen) {
      constexpr float unit_tol = 1e-4F;
      if (std::abs(v.norm_min - 1.0F) <= unit_tol && std::abs(v.norm_max - 1.0F) <= unit_tol)
        DEBUG("Peaks image \"" + H.name() + "\": all peaks are unit-norm");
      else
        DEBUG("Peaks image \"" + H.name() + "\": peak norms range from " + str(v.norm_min) + " to " + str(v.norm_max) +
              " (image encodes quantitative values)");
    }
  } catch (const Exception &e) {
    DEBUG("Peaks image \"" + H.name() + "\": validation failed: " + e[0]);
  }
}

} // namespace MR::Peaks
