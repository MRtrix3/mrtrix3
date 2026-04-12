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

#include "fixel/validate.h"

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "algo/loop.h"
#include "fixel/helpers.h"
#include "image.h"

namespace MR::Fixel {

void validate_directory(std::string_view fixel_directory_path) {

  // Verify that a valid index image and a valid directions image are present.
  // Both functions throw InvalidFixelDirectoryException on failure.
  Header index_header = find_index_header(fixel_directory_path);
  find_directions_header(fixel_directory_path);

  // Verify that the index image is well-posed.
  index_type total_nfixels;
  try {
    total_nfixels = validate_index_image(index_header.get_image<index_type>());
  } catch (Exception &e) {
    throw Exception(e, "Error in index image of fixel directory " + fixel_directory_path);
  }

  // Verify that every fixel data file in the directory contains
  // the same number of fixels as implied by the index image.
  auto dir_walker = Path::Dir(fixel_directory_path);
  std::string fname;
  while (!(fname = dir_walker.read_name()).empty()) {
    if (!Path::has_suffix(fname, supported_image_formats))
      continue;
    if (is_index_filename(fname))
      continue;
    const std::string full_path = Path::join(fixel_directory_path, fname);
    try {
      const Header H = Header::open(full_path);
      if (!is_data_file(H))
        continue;
      if (static_cast<index_type>(H.size(0)) != total_nfixels)
        throw InvalidDirectoryException("Fixel data file \"" + fname + "\"" +                           //
                                        " in directory \"" + std::string(fixel_directory_path) + "\"" + //
                                        " contains " + str(H.size(0)) + " fixels," +                    //
                                        " but the index image contains " + str(total_nfixels));         //
    } catch (InvalidDirectoryException &) {
      throw;
    } catch (...) {
    }
  }
}

index_type validate_index_image(Image<index_type> index_image) {
  // Collect all non-empty (offset, count) pairs from the index image.
  std::vector<std::pair<index_type, index_type>> entries;
  {
    for (auto l = Loop(index_image, 0, 3)(index_image); l; ++l) {
      index_image.index(3) = 0;
      const index_type count = index_image.value();
      if (!count)
        continue;
      index_image.index(3) = 1;
      const index_type offset = index_image.value();
      entries.emplace_back(offset, count);
    }
  }

  // Determine the total number of fixels implied by the index image,
  // and guard against offset + count overflow.
  index_type total_nfixels = 0;
  for (const auto &[offset, count] : entries) {
    if (offset > std::numeric_limits<index_type>::max() - count)
      throw InvalidDirectoryException(                                                           //
          "Fixel index image \"" + index_image.name() + "\"" +                                   //
          " contains an entry where offset (" + str(offset) + ") + count (" + str(count) + ")" + //
          " overflows the index type");                                                          //
    total_nfixels = std::max(total_nfixels, offset + count);
  }

  // Check whether this total number matches what is stored in the header
  try {
    const index_type nfixels_header = to<index_type>(index_image.keyval().at(n_fixels_key));
    if (to<index_type>(index_image.keyval().at(n_fixels_key)) != total_nfixels) {
      WARN("Total number of fixels indicated in header of image \"" + index_image.name() + "\"" + //
           " (" + str(nfixels_header) + ")" +                                                     //
           " does not match that indicated in image data" +                                       //
           " (" + str(total_nfixels) + ")");                                                      //
    }
  } catch (std::out_of_range) {
  }

  // Verify that every fixel index in [0, total_nfixels) is covered
  // by exactly one voxel in the index image.
  std::vector<uint32_t> fixel_counts(total_nfixels, 0);
  for (const auto &[offset, count] : entries) {
    for (index_type j = 0; j < count; ++j)
      fixel_counts[offset + j]++;
  }
  for (index_type i = 0; i < total_nfixels; ++i) {
    if (fixel_counts[i] != 1)
      throw InvalidDirectoryException("Fixel index " + str(i) + " in image \"" + index_image.name() + "\"" +      //
                                      " is covered by " + str(fixel_counts[i]) + " voxel(s) in the index image" + //
                                      " (expected exactly one)");                                                 //
  }

  return total_nfixels;
}

void debug_validate_directory(std::string_view fixel_directory_path) {
  if (App::log_level >= 3)
    validate_directory(fixel_directory_path);
}

void debug_validate_index_image(Image<index_type> index_image) {
  if (App::log_level >= 3)
    validate_index_image(index_image);
}

} // namespace MR::Fixel

namespace MR::Peaks {

void validate_header(const Header &H) {
  try {
    if (!H.datatype().is_floating_point())
      throw Exception("Does not contain floating-point data");
    if (H.datatype().is_complex())
      throw Exception("Complex data not supported in \"peaks\" format");
    try {
      check_effective_dimensionality(H, 4);
    } catch (Exception &e) {
      throw Exception(e, "Expect 4 dimensions");
    }
    if (H.size(3) % 3)
      throw Exception("Number of volumes must be a multiple of 3");
  } catch (Exception &e) {
    throw Exception(e, "Image \"" + H.name() + "\" is not a valid peaks image");
  }
}

const PeaksValidation validate_image(Image<float> image) {
  // ---------------------------------------------------------------
  // Check 1 & 2: floating-point type, 4D, volumes multiple of 3.
  // Delegate to the existing lightweight structural check.
  // ---------------------------------------------------------------
  validate_header(image);

  // Number of peaks slots per voxel.
  const ssize_t n_peaks = image.size(3) / 3;

  PeaksValidation result;
  size_t partial_nan_count = 0;
  size_t infinity_count = 0;

  for (auto l = Loop("Validating peaks image", image, 0, 3)(image); l; ++l) {
    for (ssize_t p = 0; p < n_peaks; ++p) {
      // Read the three components of peak p.
      image.index(3) = p * 3;
      const float x = image.value();
      image.index(3) = p * 3 + 1;
      const float y = image.value();
      image.index(3) = p * 3 + 2;
      const float z = image.value();

      const bool x_nan = std::isnan(x);
      const bool y_nan = std::isnan(y);
      const bool z_nan = std::isnan(z);
      const bool any_nan = x_nan || y_nan || z_nan;
      const bool all_nan = x_nan && y_nan && z_nan;

      const bool x_zero = (x == 0.0F);
      const bool y_zero = (y == 0.0F);
      const bool z_zero = (z == 0.0F);
      const bool all_zero = x_zero && y_zero && z_zero;

      const bool any_inf = std::isinf(x) || std::isinf(y) || std::isinf(z);

      // ---------------------------------------------------------------
      // Check 3: fill-value consistency.
      // A triplet is a "fill" if it is all-zero or all-NaN.
      // A triplet is a "real peak" if it is fully finite and non-zero.
      // The same fill convention must be used throughout the image.
      // ---------------------------------------------------------------
      if (all_zero || all_nan) {
        // This is a fill triplet.
        const float this_fill = all_nan ? std::numeric_limits<float>::quiet_NaN() : 0.0F;
        if (!result.fill_value.has_value()) {
          // First fill triplet seen — lock in the fill convention.
          result.fill_value = this_fill;
        } else if ((std::isnan(this_fill) ? 1 : 0) + (std::isnan(*result.fill_value) ? 1 : 0) == 1) {
          throw Exception("Peaks image \"" + image.name() + "\":" +        //
                          " mixed fill values detected" +                  //
                          " (both zero and NaN triplets are present);" +   //
                          " a single fill value must be used throughout"); //
        }
        continue; // no further checks needed for this triplet: not a fill value
      }

      // ---------------------------------------------------------------
      // Check 4: NaN-fill coherence.
      // If the fill convention is NaN, then every non-fill triplet must
      // be entirely finite (no individual NaN components are permitted).
      // We also catch partial NaN triplets regardless of fill convention,
      // because a triplet that is partly NaN and partly non-NaN cannot
      // represent either a valid peak or a valid NaN fill.
      // ---------------------------------------------------------------
      // We already know the triplet is not all-NaN (handled above).
      // So this is a partial-NaN triplet, which is always invalid.
      if (any_nan) {
        ++partial_nan_count;
        continue;
      }

      // ---------------------------------------------------------------
      // Check 5: Infinity values are not permitted.
      // ---------------------------------------------------------------
      if (any_inf) {
        ++infinity_count;
        continue;
      }

      // Track the norm range.
      const float this_norm = std::sqrt(Math::pow2(x) + Math::pow2(y) + Math::pow2(z));
      if (!std::isfinite(result.norm_min)) {
        result.norm_min = this_norm;
        result.norm_max = this_norm;
      } else {
        result.norm_min = std::min(result.norm_min, this_norm);
        result.norm_max = std::max(result.norm_max, this_norm);
      }
    }
  }

  Exception e;
  if (partial_nan_count > 0 || infinity_count > 0)
    e.push_back("Peaks image \"" + image.name() + "\":");

  if (partial_nan_count > 0)
    e.push_back(str(partial_nan_count) + " peak triplet" +                      //
                (partial_nan_count > 1 ? "s that contain" : " that contains") + //
                " a mixture of NaN and non-NaN values" +                        //
                " (a peak triplet must be either all-finite or all-NaN)");      //

  if (infinity_count > 0)
    e.push_back(str(infinity_count) + " peak triplet" +                      //
                (infinity_count > 1 ? "s that contain" : " that contains") + //
                " impermitted infinity values");                             //

  if (e.num())
    throw e;

  return result;
}

void debug_validate_image(Image<float> image) {
  validate_header(image);
  if (App::log_level < 3)
    return;
  try {
    const PeaksValidation v = validate_image(image);
    DEBUG("Peaks image \"" + image.name() + "\":" +
          (v.fill_value.has_value() ? (" fill value is " + str(*v.fill_value))
                                    : " no fill triplets detected (all voxels fully populated)"));
    if (std::isfinite(v.norm_min)) {
      constexpr float unit_tol = 1e-4F;
      DEBUG("Peaks image \"" + image.name() + "\": " +
            ((std::fabs(v.norm_min - 1.0F) <= unit_tol && std::fabs(v.norm_max - 1.0F) <= unit_tol)
                 ? "all peaks are unit-norm"
                 : ("peak norms range from " + str(v.norm_min) + " to " + str(v.norm_max) +
                    " (ie. image encodes a quantitative value per peak)")));
    } else {
      WARN("Peaks image \"" + image.name() + "\": no peaks data present");
    }
  } catch (const Exception &e) {
    throw Exception(e, "Peaks image \"" + image.name() + "\" validation failed");
  }
}

} // namespace MR::Peaks
