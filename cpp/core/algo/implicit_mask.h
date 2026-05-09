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

#pragma once

#include <cmath>

#include "algo/loop.h"
#include "filter/connected_components.h"
#include "image.h"

namespace MR {

//! \addtogroup algo
// @{

//! Controls whether voxels with only zero (finite) values are excluded from the mask
enum class ZeroExclusion {
  Enabled, //!< Exclude voxels that have at least one finite value, all of which are zero
  Disabled //!< Zero values do not contribute to voxel exclusion
};

//! Controls whether voxels containing non-finite values are excluded from the mask
enum class NonFiniteExclusion {
  Any,     //!< Exclude voxels that contain at least one non-finite value
  All,     //!< Exclude voxels only when all values are non-finite
  Disabled //!< Non-finite values do not contribute to voxel exclusion
};

//! Controls whether interior holes in the mask are filled using connected-components analysis,
//! and whether exclusion criteria are re-applied to voxels admitted by that operation
enum class HoleFilling {
  Disabled,            //!< No hole-filling is performed
  Enabled,             //!< Fill interior holes; re-apply NonFiniteExclusion only (zero-filled holes are admitted)
  EnabledWithReRemoval //!< Fill interior holes; re-apply both ZeroExclusion and NonFiniteExclusion
};

//! @}

//! Construct a binary mask from image data using configurable exclusion and hole-filling criteria
/*! The mask is a 3D scratch Image<bool> aligned to the spatial grid of \a source.
 * A voxel is initially excluded if it fails the zero test (ZeroExclusion::Enabled), the
 * non-finite test (NonFiniteExclusion), or both; otherwise it is included.
 *
 * Zero test (when ZeroExclusion::Enabled): the voxel has at least one finite value, but no
 * finite non-zero value (i.e. all finite values are zero).
 *
 * If HoleFilling is not Disabled, the mask is inverted, the largest connected component
 * (the exterior background) is retained, and the mask is inverted back — filling interior
 * holes.  Excluded voxels are then selectively re-removed according to the HoleFilling mode:
 * Enabled re-removes only those that failed the non-finite test; EnabledWithReRemoval
 * re-removes all originally-excluded voxels.
 */
template <class ImageType>
Image<bool> make_implicit_mask(const ImageType &source,
                               ZeroExclusion zero_excl,
                               NonFiniteExclusion nonfinite_excl,
                               HoleFilling hole_filling) {
  Header header3d(source);
  header3d.ndim() = 3;
  auto mask = Image<bool>::scratch(header3d, "implicit mask");

  auto img = source;
  const bool has_volumes = img.ndim() > 3;
  const bool do_hole_fill = hole_filling != HoleFilling::Disabled;

  struct Classification {
    bool valid;
    bool reexclude;
  };
  auto classify = [&]() -> Classification {
    bool has_finite_nonzero = false;
    bool has_finite = false;
    bool has_nonfinite = false;
    auto check = [&]() {
      const auto v = static_cast<double>(img.value());
      if (!std::isfinite(v)) {
        has_nonfinite = true;
      } else {
        has_finite = true;
        if (v != 0.0)
          has_finite_nonzero = true;
      }
    };
    if (has_volumes) {
      for (auto l_v = Loop(img, 3)(img); l_v; ++l_v)
        check();
    } else {
      check();
    }
    const bool zero_failed = (zero_excl == ZeroExclusion::Enabled) && has_finite && !has_finite_nonzero;
    const bool nonfinite_failed = (nonfinite_excl == NonFiniteExclusion::Any && has_nonfinite) ||
                                  (nonfinite_excl == NonFiniteExclusion::All && !has_finite);
    return {!(zero_failed || nonfinite_failed),
            (hole_filling == HoleFilling::EnabledWithReRemoval) ? (zero_failed || nonfinite_failed) : nonfinite_failed};
  };

  if (do_hole_fill) {
    auto reexclude = Image<bool>::scratch(header3d, "implicit mask re-exclusion");
    for (auto l = Loop(mask)(mask, reexclude); l; ++l) {
      img.index(0) = mask.index(0);
      img.index(1) = mask.index(1);
      img.index(2) = mask.index(2);
      const auto [valid, should_reexclude] = classify();
      mask.value() = valid;
      reexclude.value() = should_reexclude;
    }
    for (auto l = Loop(mask)(mask); l; ++l)
      mask.value() = !mask.value();
    Filter::ConnectedComponents cc_filter(mask);
    cc_filter.set_largest_only(true);
    cc_filter(mask, mask);
    for (auto l = Loop(mask)(mask); l; ++l)
      mask.value() = !mask.value();
    for (auto l = Loop(mask)(mask, reexclude); l; ++l) {
      if (reexclude.value())
        mask.value() = false;
    }
  } else {
    for (auto l = Loop(mask)(mask); l; ++l) {
      img.index(0) = mask.index(0);
      img.index(1) = mask.index(1);
      img.index(2) = mask.index(2);
      mask.value() = classify().valid;
    }
  }

  return mask;
}

} // namespace MR
