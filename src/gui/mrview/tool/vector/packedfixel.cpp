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

#include "gui/mrview/tool/vector/packedfixel.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        void PackedFixel::load_image_buffer()
        {
          size_t ndim = fixel_data->ndim ();

          if (ndim != 4)
            throw InvalidImageException ("Vector image " + filename
                                       + " should contain 4 dimensions. Instead "
                                       + str(ndim) + " found.");

          size_t dim4_len = fixel_data->size (3);

          if (dim4_len % 3)
            throw InvalidImageException ("Expecting 4th-dimension size of vector image "
                                       + filename + " to be a multiple of 3. Instead "
                                       + str(dim4_len) + " entries found.");

          for (size_t axis = 0; axis < 3; ++axis) {
            slice_fixel_indices[axis].resize (fixel_data->size (axis));
            slice_fixel_sizes  [axis].resize (fixel_data->size (axis));
            slice_fixel_counts [axis].resize (fixel_data->size (axis), 0);
          }

          const size_t n_fixels = dim4_len / 3;

          FixelValue &fixel_val_store = fixel_values["Length"];

          for (auto l = Loop(*fixel_data) (*fixel_data); l; ++l) {

            const std::array<int, 3> voxel {{ int(fixel_data->index (0)),
              int(fixel_data->index (1)), int(fixel_data->index (2)) }};

            Eigen::Vector3f pos { float(voxel[0]), float(voxel[1]), float(voxel[2]) };
            pos = transform.voxel2scanner.cast<float> () * pos;

            for (size_t f = 0; f < n_fixels; ++f) {

              // Fetch the vector components
              Eigen::Vector3f vector;
              fixel_data->index (3) = 3*f;
              vector[0] = fixel_data->value ();
              fixel_data->index (3)++;
              vector[1] = fixel_data->value ();
              fixel_data->index (3)++;
              vector[2] = fixel_data->value ();

              const float length = vector.norm ();

              pos_buffer_store.push_back (pos);
              dir_buffer_store.push_back (vector.normalized());
              fixel_val_store.add_value (length);

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
