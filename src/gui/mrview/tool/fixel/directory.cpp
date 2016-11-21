/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */

#include "gui/mrview/tool/fixel/directory.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        void Directory::load_image_buffer()
        {
          for (size_t axis = 0; axis < 3; ++axis) {
            slice_fixel_indices[axis].resize (fixel_data->size (axis));
            slice_fixel_sizes  [axis].resize (fixel_data->size (axis));
            slice_fixel_counts [axis].resize (fixel_data->size (axis), 0);
          }

          // Load fixel index image
          for (auto l = Loop(0, 3) (*fixel_data); l; ++l) {

            const std::array<int, 3> voxel {{ int(fixel_data->index (0)), int(fixel_data->index (1)), int(fixel_data->index (2)) }};
            Eigen::Vector3f pos { float(voxel[0]), float(voxel[1]), float(voxel[2]) };
            pos = transform.voxel2scanner.cast<float> () * pos;

            fixel_data->index (3) = 0;
            const size_t nfixels = fixel_data->value ();

            for (size_t f = 0; f < nfixels; ++f) {

              pos_buffer_store.push_back (pos);

              const GLint point_index = pos_buffer_store.size () - 1;

              for (size_t axis = 0; axis < 3; ++axis) {
                slice_fixel_indices[axis][voxel[axis]].push_back (point_index);
                slice_fixel_sizes  [axis][voxel[axis]].push_back (1);
                slice_fixel_counts [axis][voxel[axis]]++;
              }

              voxel_to_indices_map[voxel].push_back (point_index);
            }
          }



          // Load fixel direction images
          auto directions_image = MR::Fixel::find_directions_header (Path::dirname (fixel_data->name())).get_image<float>().with_direct_io ();
          directions_image.index (1) = 0;
          for (auto l = Loop(0, 3) (*fixel_data); l; ++l) {
            fixel_data->index (3) = 0;
            const size_t nfixels = fixel_data->value ();
            fixel_data->index (3) = 1;
            const size_t offset = fixel_data->value ();
            for (size_t f = 0; f < nfixels; ++f) {
              directions_image.index (0) = offset + f;
              dir_buffer_store.emplace_back (directions_image.row (1));
            }
          }

          // Load fixel data images keys
          // We will load the actual fixel data lazily upon request
          auto data_headers = MR::Fixel::find_data_headers (Path::dirname (fixel_data->name ()), *fixel_data);
          for (auto& header : data_headers) {

            if (header.size (1) != 1) continue;

            const auto data_key = Path::basename (header.name ());
            fixel_values[data_key];
            value_types.push_back (data_key);
            colour_types.push_back (data_key);
            threshold_types.push_back (data_key);
          }
        }

        void Directory::lazy_load_fixel_value_file (const std::string& key) const {

          // We're assuming the key corresponds to the fixel data filename
          const auto data_filepath = Path::join(Path::dirname (fixel_data->name ()), key);
          fixel_values[key].loaded = true;

          if (!Path::exists (data_filepath))
            return;

          auto H = Header::open (data_filepath);

          if (!MR::Fixel::is_data_file (H))
            return;

          auto data_image = H.get_image<float> ();

          data_image.index (1) = 0;
          for (auto l = Loop(0, 3) (*fixel_data); l; ++l) {
            fixel_data->index (3) = 0;
            const size_t nfixels = fixel_data->value ();
            fixel_data->index (3) = 1;
            const size_t offset = fixel_data->value ();

            for (size_t f = 0; f < nfixels; ++f) {
              data_image.index (0) = offset + f;
              float value = data_image.value ();
              fixel_values[key].add_value (value);
            }
          }

          fixel_values[key].initialise_windowing ();
        }


        FixelValue& Directory::get_fixel_value (const std::string& key) const {
          if (!has_values ())
            return dummy_fixel_val_state;

          FixelValue& fixel_val = fixel_values[key];
          // Buffer hasn't been loaded yet -  we do this lazily
          if (!fixel_val.loaded)
            lazy_load_fixel_value_file (key);

          return fixel_val;
        }

      }
    }
  }
}
