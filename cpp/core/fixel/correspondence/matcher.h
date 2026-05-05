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

#include <mutex>
#include <string_view>

#include "image.h"

#include "fixel/correspondence/correspondence.h"
#include "fixel/correspondence/mapping.h"

namespace MR::Fixel::Correspondence::Algorithms {
class Base;
}

namespace MR::Fixel::Correspondence {

// Functor is safe to copy-construct for multi-threading:
//   correspondence data are stored in a std::shared_ptr<>
class Matcher {

public:
  Matcher(std::string_view source_file, std::string_view target_file, std::shared_ptr<Algorithms::Base> &algorithm);

  // Input is just a dummy iterator that provides the location
  void operator()(Image<index_type> &voxel);

  // Use this to get a template image in order to loop over voxels
  Image<index_type> get_template() const { return Image<index_type>(target_index); }

  const MR::Fixel::Correspondence::Mapping &get_mapping() const {
    assert(mapping);
    return *mapping;
  }

  size_t num_source_fixels() const { return source_data.size(0); }
  size_t num_target_fixels() const { return target_data.size(0); }

  void export_remapped(std::string_view dirname);

private:
  std::shared_ptr<Algorithms::Base> algorithm;

  Image<index_type> source_index, target_index;
  Image<float> source_directions, target_directions, remapped_directions;
  Image<float> source_data, target_data, remapped_data;

  std::shared_ptr<MR::Fixel::Correspondence::Mapping> mapping;

#ifdef FIXELCORRESPONDENCE_TEST_COMBINATORICS
  std::shared_ptr<std::mutex> mutex;
  static uint64_t max_computed_combinations;
#endif
};

#ifdef FIXELCORRESPONDENCE_TEST_COMBINATORICS
uint64_t Matcher::max_computed_combinations = 0;
#endif

} // namespace MR::Fixel::Correspondence
