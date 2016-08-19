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

#include "gui/mrview/tool/vector/fixelfolder.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        void FixelFolder::load_image_buffer()
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
          auto directions_image = FixelFormat::find_directions_header (Path::dirname (fixel_data->name ()), *fixel_data).get_image<float> ().with_direct_io ();
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

          // Load fixel data images
          auto data_headers = FixelFormat::find_data_headers (Path::dirname (fixel_data->name ()), *fixel_data);
          for (auto& header : data_headers) {

            if (header.size (1) != 1) continue;

            auto data_image = header.get_image<float> ();
            const auto data_key = Path::basename (header.name ());
            fixel_values[data_key];
            value_types.push_back (data_key);
            colour_types.push_back (data_key);
            threshold_types.push_back (data_key);

            data_image.index (1) = 0;
            for (auto l = Loop(0, 3) (*fixel_data); l; ++l) {
              fixel_data->index (3) = 0;
              const size_t nfixels = fixel_data->value ();
              fixel_data->index (3) = 1;
              const size_t offset = fixel_data->value ();

              for (size_t f = 0; f < nfixels; ++f) {
                data_image.index (0) = offset + f;
                float value = data_image.value ();
                fixel_values[data_key].add_value (value);
              }
            }

          }

          if (!fixel_values.size ())
            throw InvalidImageException ("Fixel index image " + fixel_data->name () + " has no associated image data files");
        }

      }
    }
  }
}
