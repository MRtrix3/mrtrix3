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

#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "algo/loop.h"
#include "app.h"
#include "fixel/helpers.h"

namespace MR::Fixel {

//! Perform thorough validation of a fixel directory.
//! Checks that:
//!   - a valid index image is present;
//!   - a valid directions image is present;
//!   - every fixel index is covered by exactly one voxel in the index image;
//!   - all fixel data files contain the same number of fixels as the index image.
//! Throws InvalidFixelDirectoryException on any failure.
FORCE_INLINE void validate_fixel_directory(std::string_view fixel_directory_path) {

  // Verify that a valid index image and a valid directions image are present.
  // Both functions throw InvalidFixelDirectoryException on failure.
  Header index_header = find_index_header(fixel_directory_path);
  find_directions_header(fixel_directory_path);

  // Collect all non-empty (offset, count) pairs from the index image.
  std::vector<std::pair<index_type, index_type>> entries;
  {
    auto index_image = index_header.get_image<index_type>();
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
      throw InvalidFixelDirectoryException("fixel index image in directory \"" + std::string(fixel_directory_path) +
                                           "\" contains an entry where offset (" + str(offset) + ") + count (" +
                                           str(count) + ") overflows the index type");
    total_nfixels = std::max(total_nfixels, offset + count);
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
      throw InvalidFixelDirectoryException(
          "fixel index " + str(i) + " in directory \"" + std::string(fixel_directory_path) + "\" is covered by " +
          str(fixel_counts[i]) + " voxel(s) in the index image (expected exactly one)");
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
        throw InvalidFixelDirectoryException("fixel data file \"" + fname + "\" in directory \"" +
                                             std::string(fixel_directory_path) + "\" contains " + str(H.size(0)) +
                                             " fixel(s),"
                                             " but the index image implies " +
                                             str(total_nfixels));
    } catch (InvalidFixelDirectoryException &) {
      throw;
    } catch (...) {
    }
  }
}

//! Call validate_fixel_directory() only when running in debug mode (log_level >= 3).
//! Intended for use in other fixel commands to add lightweight validation in debug builds.
FORCE_INLINE void debug_validate_fixel_directory(std::string_view fixel_directory_path) {
  if (App::log_level >= 3)
    validate_fixel_directory(fixel_directory_path);
}

} // namespace MR::Fixel
