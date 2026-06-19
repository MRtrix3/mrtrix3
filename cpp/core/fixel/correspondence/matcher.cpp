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

#include "fixel/correspondence/matcher.h"

#include <string>

#include "fixel/correspondence/algorithms/base.h"
#include "fixel/correspondence/fixel.h"
#include "fixel/fixel.h"
#include "fixel/helpers.h"
#include "header.h"

namespace MR::Fixel::Correspondence {

Matcher::Matcher(const std::filesystem::path &source_file,
                 const std::filesystem::path &target_file,
                 std::shared_ptr<Algorithms::Base> &algorithm)
    : algorithm(algorithm)
#ifdef FIXELCORRESPONDENCE_TEST_COMBINATORICS
      ,
      mutex(new std::mutex)
#endif
{
  if (std::filesystem::is_directory(source_file))
    throw Exception(
        "Please input the source fixel data file to be used during fixel correspondence; not the fixel directory");
  Header source_header(Header::open(source_file));
  if (!MR::Fixel::is_data_file(source_header))
    throw Exception("Source input image is not a fixel data file");

  const std::filesystem::path source_directory = MR::Fixel::get_fixel_directory(source_file);
  source_index = MR::Fixel::find_index_header(source_directory).get_image<MR::Fixel::index_type>();
  source_directions = MR::Fixel::find_directions_header(source_directory).get_image<float>();
  source_data = source_header.get_image<float>();
  MR::Fixel::check_fixel_size(source_index, source_data);

  if (std::filesystem::is_directory(target_file))
    throw Exception(
        "Please input the target fixel data file to be used during fixel correspondence; not the fixel directory");
  Header target_header(Header::open(target_file));
  if (!MR::Fixel::is_data_file(target_header))
    throw Exception("Target input image is not a fixel data file");

  const std::filesystem::path target_directory = MR::Fixel::get_fixel_directory(target_file);
  target_index = MR::Fixel::find_index_header(target_directory).get_image<MR::Fixel::index_type>();
  target_directions = MR::Fixel::find_directions_header(target_directory).get_image<float>();
  target_data = target_header.get_image<float>();
  MR::Fixel::check_fixel_size(target_index, target_data);

  // Source and target images need to match spatially,
  //   but do not need to contain the same number of fixels
  check_dimensions(source_index, target_index);
  check_voxel_grids_match_in_scanner_space(source_index, target_index);

  remapped_directions = Image<float>::scratch(target_directions, "scratch image for remapped fixel directions");
  remapped_data = Image<float>::scratch(target_data, "scratch image for remapped fixel densities");

  mapping.reset(new MR::Fixel::Correspondence::Mapping(MR::Fixel::get_number_of_fixels(source_index),
                                                       MR::Fixel::get_number_of_fixels(target_index)));
}

void Matcher::load_voxel(Image<MR::Fixel::index_type> &voxel,
                         std::vector<Correspondence::Fixel> &source_fixels,
                         std::vector<Correspondence::Fixel> &target_fixels,
                         index_type &offset_source,
                         index_type &offset_target) {
  assign_pos_of(voxel, 0, 3).to(source_index, target_index);
  source_index.index(3) = target_index.index(3) = 0;
  const index_type nfixels_source = source_index.value();
  const index_type nfixels_target = target_index.value();
  source_index.index(3) = target_index.index(3) = 1;
  offset_source = source_index.value();
  offset_target = target_index.value();

  // Perform an initial load of the fixel information; this can
  //   then be palmed off to the appropriate algorithm
  // By pre-loading into vectors, can have fixels in both spaces indexed from zero
  //   during the correspondence determination
  source_fixels.clear();
  target_fixels.clear();
  for (index_type i = 0; i != nfixels_source; ++i) {
    source_directions.index(0) = source_data.index(0) = offset_source + i;
    source_fixels.push_back(Correspondence::Fixel(source_directions.row(1), source_data.value()));
  }
  for (index_type i = 0; i != nfixels_target; ++i) {
    target_directions.index(0) = target_data.index(0) = offset_target + i;
    target_fixels.push_back(Correspondence::Fixel(target_directions.row(1), target_data.value()));
  }
}

void Matcher::operator()(Image<MR::Fixel::index_type> &voxel) {
  std::vector<Correspondence::Fixel> source_fixels, target_fixels;
  index_type offset_source, offset_target;
  load_voxel(voxel, source_fixels, target_fixels, offset_source, offset_target);
  const index_type nfixels_target = target_fixels.size();

  std::vector<std::vector<Mapping::Entry>> M;
  if (target_fixels.size()) {
    if (source_fixels.size())
      M = (*algorithm)({static_cast<uint32_t>(voxel.index(0)),
                        static_cast<uint32_t>(voxel.index(1)),
                        static_cast<uint32_t>(voxel.index(2))},
                       source_fixels,
                       target_fixels);
    else
      M.assign(target_fixels.size(), std::vector<Mapping::Entry>());
  }

  for (index_type it = 0; it != nfixels_target; ++it) {
    target_directions.index(0) = remapped_directions.index(0) = remapped_data.index(0) = offset_target + it;
    dir_t direction(0.0f, 0.0f, 0.0f);
    float density = 0.0f;
    for (const auto &e : M[it]) {
      direction += source_fixels[e.index].density() * source_fixels[e.index].dir() *
                   (source_fixels[e.index].dot(target_fixels[it]) > 0.0f ? 1.0f : -1.0f);
      density += source_fixels[e.index].density();
    }
    remapped_directions.row(1) = direction.normalized();
    remapped_data.value() = density;
  }

  // When writing, need to now deal with the offset to the first fixel in the
  //   voxel for each of the two images
  assert(M.size() == nfixels_target);
  for (index_type i = 0; i != nfixels_target; ++i) {
    std::vector<Mapping::Entry> entries;
    entries.reserve(M[i].size());
    for (const auto &e : M[i])
      entries.push_back({e.index + offset_source, e.weight});
    (*mapping)[offset_target + i] = entries;
  }
}

void Matcher::export_remapped(const std::filesystem::path &dirname) {
  MR::Fixel::check_fixel_directory(dirname, true, true);
  Image<MR::Fixel::index_type> out_index(Image<MR::Fixel::index_type>::create(dirname / "index.mif", target_index));
  copy(target_index, out_index);
  Image<float> out_directions(Image<float>::create(dirname / "directions.mif", target_directions));
  copy(remapped_directions, out_directions);
  Image<float> out_data(Image<float>::create(dirname / "fd.mif", target_data));
  copy(remapped_data, out_data);
}

} // namespace MR::Fixel::Correspondence
