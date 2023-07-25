/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "fixel/correspondence/matcher.h"

#include "header.h"
#include "fixel/helpers.h"
#include "fixel/correspondence/algorithms/base.h"
#include "fixel/correspondence/fixel.h"


namespace MR {
  namespace Fixel {
    namespace Correspondence {



      Matcher::Matcher (const std::string& source_file,
                        const std::string& target_file,
                        std::shared_ptr<Algorithms::Base>& algorithm) :
          algorithm (algorithm)
#ifdef FIXELCORRESPONDENCE_TEST_COMBINATORICS
          , mutex (new std::mutex)
#endif
      {
        if (Path::is_dir (source_file))
          throw Exception ("Please input the source fixel data file to be used during fixel correspondence; not the fixel directory");
        Header source_header (Header::open (source_file));
        if (!MR::Fixel::is_data_file (source_header))
          throw Exception ("Source input image is not a fixel data file");

        const std::string source_directory = MR::Fixel::get_fixel_directory (source_file);
        source_index = MR::Fixel::find_index_header (source_directory).get_image<uint32_t>();
        source_directions = MR::Fixel::find_directions_header (source_directory).get_image<float>();
        source_data = source_header.get_image<float>();
        MR::Fixel::check_fixel_size (source_index, source_data);

        if (Path::is_dir (target_file))
          throw Exception ("Please input the target fixel data file to be used during fixel correspondence; not the fixel directory");
        Header target_header (Header::open (target_file));
        if (!MR::Fixel::is_data_file (target_header))
          throw Exception ("Target input image is not a fixel data file");

        const std::string target_directory = MR::Fixel::get_fixel_directory (target_file);
        target_index = MR::Fixel::find_index_header (target_directory).get_image<uint32_t>();
        target_directions = MR::Fixel::find_directions_header (target_directory).get_image<float>();
        target_data = target_header.get_image<float>();
        MR::Fixel::check_fixel_size (target_index, target_data);

        // Source and target images need to match spatially,
        //   but do not need to contain the same number of fixels
        check_dimensions (source_index, target_index);
        check_voxel_grids_match_in_scanner_space (source_index, target_index);

        remapped_directions = Image<float>::scratch (target_directions, "scratch image for remapped fixel directions");
        remapped_data = Image<float>::scratch (target_data, "scratch image for remapped fixel densities");

        mapping.reset (new MR::Fixel::Correspondence::Mapping (MR::Fixel::get_number_of_fixels (source_index),
                                                               MR::Fixel::get_number_of_fixels (target_index)));
      }



      void Matcher::operator() (Image<uint32_t>& voxel)
      {
        assign_pos_of (voxel, 0, 3).to (source_index, target_index);
        source_index.index(3) = target_index.index(3) = 0;
        const uint32_t nfixels_source = source_index.value();
        const uint32_t nfixels_target = target_index.value();
        source_index.index(3) = target_index.index(3) = 1;
        const uint32_t offset_source = source_index.value();
        const uint32_t offset_target = target_index.value();

        // Perform an initial load of the fixel information; this can
        //   then be palmed off to the appropriate algorithm
        // By pre-loading into vectors, can have fixels in both spaces indexed from zero
        //   during the correspondence determination
        vector<Correspondence::Fixel> source_fixels, target_fixels;
        for (uint32_t i = 0; i != nfixels_source; ++i) {
          source_directions.index(0) = source_data.index(0) = offset_source + i;
          source_fixels.push_back (Correspondence::Fixel (source_directions.row(1), source_data.value()));
        }
        for (uint32_t i = 0; i != nfixels_target; ++i) {
          target_directions.index(0) = target_data.index(0) = offset_target + i;
          target_fixels.push_back (Correspondence::Fixel (target_directions.row(1), target_data.value()));
        }

        vector< vector<uint32_t> > M;
        if (target_fixels.size()) {
          if (source_fixels.size())
            M = (*algorithm) ({uint32_t(voxel.index(0)), uint32_t(voxel.index(1)), uint32_t(voxel.index(2))}, source_fixels, target_fixels);
          else
            M.assign (target_fixels.size(), vector<uint32_t>());
        }

        // TODO Generate the set of remapped subject fixels
        Eigen::Array<uint8_t, Eigen::Dynamic, 1> objectives_per_source_fixel (Eigen::Array<uint8_t, Eigen::Dynamic, 1>::Zero (nfixels_source));
        for (uint32_t it = 0; it != nfixels_target; ++it) {
          for (auto is : M[it])
            ++objectives_per_source_fixel[is];
        }
        const Eigen::Array<float, Eigen::Dynamic, 1> source_fixel_multipliers (1.0 / objectives_per_source_fixel.cast<float>());
        for (uint32_t it = 0; it != nfixels_target; ++it) {
          target_directions.index(0) = remapped_directions.index(0) = remapped_data.index(0) = offset_target + it;
          dir_t direction (0.0f, 0.0f, 0.0f);
          float density = 0.0f;
          for (auto is : M[it]) {
            direction += source_fixels[is].density() * source_fixels[is].dir() * (source_fixels[is].dot(target_fixels[it]) > 0.0f ? 1.0f : -1.0f);
            density += source_fixels[is].density();
          }
          remapped_directions.row(1) = direction.normalized();
          remapped_data.value() = density;
        }

        // When writing, need to now deal with the offset to the first fixel in the
        //   voxel for each of the two images
        assert (M.size() == nfixels_target);
        for (uint32_t i = 0; i != nfixels_target; ++i) {
          for (uint32_t j = 0; j != M[i].size(); ++j)
            M[i][j] += offset_source;
          (*mapping)[offset_target + i] = M[i];
        }
      }



      void Matcher::export_remapped (const std::string& dirname)
      {
        MR::Fixel::check_fixel_directory (dirname, true, true);
        Image<uint32_t> out_index (Image<uint32_t>::create (Path::join (dirname, "index.mif"), target_index));
        copy (target_index, out_index);
        Image<float> out_directions (Image<float>::create (Path::join (dirname, "directions.mif"), target_directions));
        copy (remapped_directions, out_directions);
        Image<float> out_data (Image<float>::create (Path::join (dirname, "fd.mif"), target_data));
        copy (remapped_data, out_data);
      }



    }
  }
}
