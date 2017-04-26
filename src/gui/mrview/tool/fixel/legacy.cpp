/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#include "gui/mrview/tool/fixel/legacy.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        void Legacy::load_image_buffer()
        {
          for (size_t axis = 0; axis < 3; ++axis) {
            const size_t axis_size = fixel_data->size (axis);
            slice_fixel_indices[axis].resize (axis_size);
            slice_fixel_sizes  [axis].resize (axis_size);
            slice_fixel_counts [axis].resize (axis_size, 0);
          }

          FixelValue& fixel_val_store = fixel_values[value_types[1]];
          FixelValue& fixel_secondary_val_store = fixel_values[value_types[2]];

          for (auto l = Loop (*fixel_data) (*fixel_data); l; ++l) {

            const std::array<int, 3> voxel {{ int(fixel_data->index (0)),
              int(fixel_data->index (1)), int(fixel_data->index (2)) }};

            Eigen::Vector3f pos { float(voxel[0]), float(voxel[1]), float(voxel[2]) };
            pos = transform.voxel2scanner.cast<float> () * pos;

            for (size_t f = 0; f != fixel_data->value ().size (); ++f) {

              auto val = fixel_data->value ()[f].size;
              auto secondary_val = fixel_data->value ()[f].value;

              pos_buffer_store.push_back (pos);
              dir_buffer_store.push_back (fixel_data->value ()[f].dir);

              fixel_val_store.add_value (val);
              fixel_secondary_val_store.add_value (secondary_val);

              GLint point_index = pos_buffer_store.size () - 1;

              for (size_t axis = 0; axis < 3; ++axis) {
                slice_fixel_indices[axis][voxel[axis]].push_back (point_index);
                slice_fixel_sizes  [axis][voxel[axis]].push_back (1);
                slice_fixel_counts [axis][voxel[axis]]++;
              }

              voxel_to_indices_map[voxel].push_back (point_index);
            }
          }
        }
      }
    }
  }
}
