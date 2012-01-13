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

#ifndef __filter_lcc_h__
#define __filter_lcc_h__

#include "point.h"
#include "image/scratch.h"
#include "dataset/copy.h"
#include "dataset/nav.h"
#include "filter/base.h"


namespace MR
{
  namespace Filter
  {

  template <class value_type, class InputSet, class OutputSet> class LargestConnectedComponent :
    public Base< InputSet, OutputSet >
    {

      public:
      LargestConnectedComponent (InputSet& DataSet, const std::string& message) :
        Base<InputSet, OutputSet>(DataSet),
        in (DataSet),
        progress (message) { }


      typedef Point<int> Voxel;


      void execute (OutputSet& output) {

        // Force calling the templated constructor instead of the copy-constructor
        Image::Scratch<bool> visited_data (in.header(), "visited");
        Image::Scratch<bool>::voxel_type visited (visited_data);
        size_t largest_mask_size = 0;

        Voxel seed (0, 0, 0);

        std::vector<Voxel> adj_voxels;
        adj_voxels.reserve (6);
        adj_voxels.push_back (Voxel (-1,  0,  0));
        adj_voxels.push_back (Voxel (+1,  0,  0));
        adj_voxels.push_back (Voxel ( 0, -1,  0));
        adj_voxels.push_back (Voxel ( 0, +1,  0));
        adj_voxels.push_back (Voxel ( 0,  0, -1));
        adj_voxels.push_back (Voxel ( 0,  0, +1));

        for (seed[2] = 0; seed[2] != in.dim (2); ++seed[2]) {
          for (seed[1] = 0; seed[1] != in.dim (1); ++seed[1]) {
            for (seed[0] = 0; seed[0] != in.dim (0); ++seed[0]) {
              if (!DataSet::Nav::get_value_at_pos (visited, seed) && DataSet::Nav::get_value_at_pos (in, seed)) {

                visited.value() = true;
                Image::Scratch<value_type> local_mask_data (in.header(), "local_mask");
                typename Image::Scratch<value_type>::voxel_type local_mask (local_mask_data);
                DataSet::Nav::set_value_at_pos (local_mask, seed, (value_type)in.value());
                size_t local_mask_size = 1;

                std::vector<Voxel> to_expand (1, seed);

                do {

                  const Voxel v = to_expand.back();
                  to_expand.pop_back();

                  for (std::vector<Voxel>::const_iterator step = adj_voxels.begin(); step != adj_voxels.end(); ++step) {
                    Voxel to_test (v);
                    to_test += *step;
                    if (DataSet::Nav::within_bounds (visited, to_test) && !DataSet::Nav::get_value_at_pos (visited, to_test)) {
                      if (DataSet::Nav::get_value_at_pos (in, to_test)) {
                        DataSet::Nav::set_value_at_pos (visited, to_test, true);
                        DataSet::Nav::set_value_at_pos (local_mask, to_test, (value_type)in.value());
                        ++local_mask_size;
                        to_expand.push_back (to_test);
                      }
                    }
                  }

                } while (to_expand.size());

                if (local_mask_size > largest_mask_size) {
                  largest_mask_size = local_mask_size;
                  DataSet::copy (output, local_mask);
                }

                ++progress;

              }
            }
          }
        }

      }


      private:
        InputSet& in;
        ProgressBar progress;

    };

  }
}



#endif
