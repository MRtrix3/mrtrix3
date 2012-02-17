/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 15/12/2011

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */

#ifndef __image_filter_lcc_h__
#define __image_filter_lcc_h__

#include "point.h"
#include "image/buffer_scratch.h"
#include "image/copy.h"
#include "image/nav.h"


namespace MR
{
  namespace Image
  {
    namespace Filter
    {

      class LargestConnectedComponent : public ConstInfo
      {

        public:
        template <class InputVoxelType> 
          LargestConnectedComponent (const InputVoxelType& in, const std::string& message) :
            ConstInfo (in),
            progress (message) { }


        typedef Point<int> voxel_type;


        template <class InputVoxelType, class OutputVoxelType>
          void operator() (InputVoxelType& input, OutputVoxelType& output) {

          typedef typename InputVoxelType::value_type value_type;

          // Force calling the templated constructor instead of the copy-constructor
          BufferScratch<bool> visited_data (input, "visited");
          BufferScratch<bool>::voxel_type visited (visited_data);
          size_t largest_mask_size = 0;

          voxel_type seed (0, 0, 0);

          std::vector<voxel_type> adj_voxels;
          adj_voxels.reserve (6);
          adj_voxels.push_back (voxel_type (-1,  0,  0));
          adj_voxels.push_back (voxel_type (+1,  0,  0));
          adj_voxels.push_back (voxel_type ( 0, -1,  0));
          adj_voxels.push_back (voxel_type ( 0, +1,  0));
          adj_voxels.push_back (voxel_type ( 0,  0, -1));
          adj_voxels.push_back (voxel_type ( 0,  0, +1));

          for (seed[2] = 0; seed[2] != input.dim (2); ++seed[2]) {
            for (seed[1] = 0; seed[1] != input.dim (1); ++seed[1]) {
              for (seed[0] = 0; seed[0] != input.dim (0); ++seed[0]) {
                if (!Image::Nav::get_value_at_pos (visited, seed) && Image::Nav::get_value_at_pos (input, seed)) {

                  visited.value() = true;
                  BufferScratch<value_type> local_mask_data (input, "local_mask");
                  typename BufferScratch<value_type>::voxel_type local_mask (local_mask_data);
                  Image::Nav::set_value_at_pos (local_mask, seed, (value_type)input.value());
                  size_t local_mask_size = 1;

                  std::vector<voxel_type> to_expand (1, seed);

                  do {

                    const voxel_type v = to_expand.back();
                    to_expand.pop_back();

                    for (std::vector<voxel_type>::const_iterator step = adj_voxels.begin(); step != adj_voxels.end(); ++step) {
                      voxel_type to_test (v);
                      to_test += *step;
                      if (Image::Nav::within_bounds (visited, to_test) && !Image::Nav::get_value_at_pos (visited, to_test)) {
                        if (Image::Nav::get_value_at_pos (input, to_test)) {
                          Image::Nav::set_value_at_pos (visited, to_test, true);
                          Image::Nav::set_value_at_pos (local_mask, to_test, (value_type)input.value());
                          ++local_mask_size;
                          to_expand.push_back (to_test);
                        }
                      }
                    }

                  } while (to_expand.size());

                  if (local_mask_size > largest_mask_size) {
                    largest_mask_size = local_mask_size;
                    Image::copy (local_mask, output);
                  }

                  ++progress;

                }
              }
            }
          }

        }


        private:
          ProgressBar progress;

      };

    }
  }
}


#endif
