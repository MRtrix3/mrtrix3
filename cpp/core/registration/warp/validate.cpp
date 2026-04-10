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

WarpFormat validate_header(const Header &H) {

  // ---------------------------------------------------------------
  // Check 1: the image must be of floating-point type.
  // ---------------------------------------------------------------
  if (!H.datatype().is_floating_point())
    throw Exception("Warp image \"" + H.name() + "\":" +          //
                    " expected a floating-point data type" +      //
                    " (got " + H.datatype().description() + ")"); //

  // ---------------------------------------------------------------
  // Check 2: structural dimensions.
  //   - 4D with size(3) == 3: simple warp (displacement or deformation field)
  //   - 5D with size(3) == 3 and size(4) == 4: full warp field
  //   - anything else: hard error
  // ---------------------------------------------------------------
  if (H.ndim() == 4) {
    if (H.size(3) != 3)
      throw Exception("Warp image \"" + H.name() + "\":" +                     //
                      " a 4D warp image (displacement or deformation field)" + //
                      " must have exactly 3 volumes in the 4th dimension" +    //
                      " (found " + str(H.size(3)) + ")");                      //
    return WarpFormat::Simple;
  }
  if (H.ndim() == 5) {
    if (H.size(3) != 3)
      throw Exception("Warp image \"" + H.name() + "\":" +                            //
                      " a 5D full warp image must have exactly 3 volumes" +           //
                      " in the 4th dimension (x/y/z components of each warp field)" + //
                      " (found " + str(H.size(3)) + ")");                             //
    if (H.size(4) != 4)
      throw Exception("Warp image \"" + H.name() + "\":" +                        //
                      " a 5D full warp image must have exactly 4 volume groups" + //
                      " in the 5th dimension (found " + str(H.size(4)) + ")");    //
    // ---------------------------------------------------------------
    // Check 3: for full warp images, the header must contain the
    // "linear1" and "linear2" linear transform keys.
    // ---------------------------------------------------------------
    if (H.keyval().find("linear1") == H.keyval().end())
      throw Exception("Warp image \"" + H.name() + "\":" +                   //
                      " full warp image header is missing the \"linear1\"" + //
                      " linear transform field");                            //
    if (H.keyval().find("linear2") == H.keyval().end())
      throw Exception("Warp image \"" + H.name() + "\":" +                   //
                      " full warp image header is missing the \"linear2\"" + //
                      " linear transform field");                            //

    return WarpFormat::Full;
  }
  throw Exception("Warp image \"" + H.name() + "\":" +                         //
                  " expected a 4D image (displacement or deformation field)" + //
                  " or a 5D image (full warp field)" +                         //
                  " (found " + str(H.ndim()) + " dimensions)");                //
}

// Enable accumulation of counts of NaN fill in image data
namespace {
struct Compare {
  template <typename ValueType> bool operator()(const ValueType a, const ValueType b) const {
    if (std::isnan(a) && std::isnan(b))
      return false;
    return a < b;
  };
};
} // namespace

template <typename ValueType> WarpValidation validate_image(Image<ValueType> image) {

  constexpr size_t minratio_voxels_to_fill = 10000;
  constexpr size_t minratio_firstfill_to_secondfill = 100;

  WarpValidation result;
  result.format = validate_header(image);

  // ---------------------------------------------------------------
  // Checks 4, 5 & 6: scan every warp triplet.
  //
  // For a simple (4D) warp there is one triplet per spatial voxel,
  // formed by dim-3 indices 0, 1, 2.
  //
  // For a full (5D) warp there are four triplets per spatial voxel
  // (one per volume group in dim 4), each formed by dim-3 indices 0, 1, 2.
  //
  // A triplet may be a fill value (all-equal or all-NaN)
  // or a data value (all-finite).
  // The fill convention must be consistent throughout.
  //
  // ---------------------------------------------------------------
  const bool is_full = (result.format == WarpFormat::Full);
  const ssize_t n_groups = is_full ? 4 : 1;

  std::map<ValueType, size_t, Compare> fill_counts;
  size_t finitemix_count = 0;

  for (auto l = Loop("Validating warp image", image, 0, 3)(image); l; ++l) {
    for (ssize_t g = 0; g < n_groups; ++g) {
      if (is_full)
        image.index(4) = g;

      image.index(3) = 0;
      const ValueType x = image.value();
      image.index(3) = 1;
      const ValueType y = image.value();
      image.index(3) = 2;
      const ValueType z = image.value();

      const bool x_finite = std::isfinite(x);
      const bool y_finite = std::isfinite(y);
      const bool z_finite = std::isfinite(z);
      const bool any_nonfinite = !x_finite || !y_finite || !z_finite;
      const bool all_nonfinite = !x_finite && !y_finite && !z_finite;
      const bool potential_fill = (std::isnan(x) && std::isnan(y) && std::isnan(z)) || ((x == z) && (y == z));

      // ---------------------------------------------------------------
      // Check 4: fill-value consistency.
      // Triplets that are all-equal or all-NaN are treated as fill.
      // The same fill convention must be used throughout the image.
      // ---------------------------------------------------------------
      if (potential_fill) {
        auto existing_fill = fill_counts.find(x);
        if (existing_fill == fill_counts.end())
          fill_counts.insert({x, 1});
        else
          ++existing_fill->second;
        continue;
      }

      // ---------------------------------------------------------------
      // Check 5: Nonfinite-fill coherence.
      // A triplet must be either all non-finite or all-finite.
      // Any partially-finite triplet is invalid
      // regardless of which fill convention the image uses.
      // ---------------------------------------------------------------
      if (any_nonfinite)
        ++finitemix_count;
    }
  }

  // ---------------------------------------------------------------
  // Check 4: fill-value consistency.
  // Triplets that are all-equal or all-NaN are treated as fill.
  // The same fill convention must be used throughout the image.
  // ---------------------------------------------------------------
  // TODO Accept a fill value as long as it is a clear majority
  // (there is a small chance that there may exist a voxel
  //  where all three finite values are equivalent,
  //  but the number of such voxels is expected to be way smaller than the true fill)
  if (!fill_counts.empty()) {
    std::map<size_t, ValueType> sort_fills;
    for (const auto &value_count : fill_counts)
      sort_fills.insert({value_count.second, value_count.first});
    // Only set this as the fill value
    //   if the fraction of the image occupied by that value is larger than some fraction
    if (voxel_count(image) / sort_fills.begin()->first > minratio_voxels_to_fill) {
      result.fill_value = sort_fills.begin()->second;
      if (fill_counts.size() > 1) {
        auto second_fill = sort_fills.begin();
        std::advance(second_fill, 1);
        if (sort_fills.begin()->first / second_fill->first < minratio_firstfill_to_secondfill)
          throw Exception("Warp image \"" + image.name() + "\":" +                                 //
                          " Unable to unambiguously determine fill value" +                        //
                          " due to presence of different voxels with different constant values;" + //
                          " a single fill convention must be used throughout");                    //
      } else {
        DEBUG("Warp image \"" + image.name() + "\": " +                //
              "Fill value of " + str(*result.fill_value) + " chosen" + //
              " despite detection of multiple potential fill values"); //
      }
    } else {
      DEBUG("Warp image \"" + image.name() + "\": " +                              //
            "No explicit fill value chosen" +                                      //
            " due to too few voxels (" + str(sort_fills.begin()->first) + ")" +    //
            "containing candidate fill value " + str(sort_fills.begin()->second)); //
    }
  }

  if (finitemix_count > 0)
    throw Exception("Warp image \"" + image.name() + "\":" +                                              //
                    str(finitemix_count) + " voxel(s) contain(s) a mix of finite and non-finite values" + //
                    " (a warp triplet must be either all-finite, or an all-nonfinite fill)");             //

  return result;
}
template WarpValidation validate_image<float>(Image<float>);
template WarpValidation validate_image<double>(Image<double>);

template <typename ValueType> void debug_validate_image(Image<ValueType> image) {
  validate_header(image);
  if (App::log_level < 3)
    return;
  try {
    const WarpValidation v = validate_image(image);
    const std::string fmt = v.format == WarpFormat::Simple ? "simple (displacement or deformation field)" : "full warp";
    DEBUG("Warp image \"" + image.name() + "\": " + fmt + " format");
    if (v.fill_value.has_value())
      DEBUG("Warp image \"" + image.name() + "\": fill value is " + str(*v.fill_value));
    else
      DEBUG("Warp image \"" + image.name() + "\": no fill value determined");
  } catch (const Exception &e) {
    throw Exception(e, "Warp image \"" + image.name() + "\" validation failed");
  }
}
template void debug_validate_image<float>(Image<float>);
template void debug_validate_image<double>(Image<double>);

} // namespace MR::Registration::Warp
