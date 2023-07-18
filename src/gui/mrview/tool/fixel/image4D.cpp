/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "gui/mrview/tool/fixel/image4D.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        void Image4D::load_image_buffer()
        {
          size_t ndim = fixel_data->ndim ();

          if (ndim < 4)
            throw InvalidImageException ("Vector image " + filename
                                       + " should contain 4 dimensions. Instead "
                                       + str(ndim) + " found.");

          const size_t dim4_len = fixel_data->size (3);

          if (dim4_len % 3)
            throw InvalidImageException ("Expecting 4th-dimension size of vector image "
                                       + filename + " to be a multiple of 3. Instead "
                                       + str(dim4_len) + " entries found.");

          for (size_t axis = 0; axis < 3; ++axis) {
            slice_fixel_indices[axis].resize (fixel_data->size (axis));
            slice_fixel_sizes  [axis].resize (fixel_data->size (axis));
            slice_fixel_counts [axis].resize (fixel_data->size (axis), 0);
          }

          reload_image_buffer();
        }





        void Image4D::reload_image_buffer ()
        {
          const size_t dim4_len = fixel_data->size (3);
          const size_t n_fixels = dim4_len / 3;

          FixelValue &fixel_val_store = fixel_values["Length"];

          pos_buffer_store.clear();
          dir_buffer_store.clear();
          fixel_val_store.clear();

          for (size_t axis = 0; axis < 3; ++axis) {
            std::fill (slice_fixel_indices[axis].begin(), slice_fixel_indices[axis].end(), vector<GLint>());
            std::fill (slice_fixel_sizes[axis].begin(), slice_fixel_sizes[axis].end(), vector<GLsizei>());
            std::fill (slice_fixel_counts[axis].begin(), slice_fixel_counts[axis].end(), 0);
          }

          for (auto l = Loop(*fixel_data, 0, 3) (*fixel_data); l; ++l) {

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

          dir_buffer_dirty = true;
          value_buffer_dirty = true;
          colour_buffer_dirty = true;
          threshold_buffer_dirty = true;
        }


        void Image4D::update_image_buffers ()
        {
          if (trackable()) {
            ssize_t target_volume = 0;
            if (tracking) {
              if (Window::main->image()) {
                const auto image = Window::main->image()->image;
                if (image.ndim() >= 4)
                  target_volume = image.index(3);
                target_volume = std::min (target_volume, fixel_data->size(4)-1);
              }
            }
            if (fixel_data->index(4) != target_volume) {
              fixel_data->index(4) = target_volume;
              reload_image_buffer();
            }
          }
          BaseFixel::update_image_buffers();
        }


      }
    }
  }
}
