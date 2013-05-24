/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2012.

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


#include "dwi/tractography/connectomics/tck2nodes.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectomics {





node_t Tck2nodes_voxel::select_node (const std::vector< Point<float> >& tck, VoxelType& voxel, bool end)
{

  const Point<float>& p (end ? tck.back() : tck.front());
  const Point<float> v_float = transform.scanner2voxel (p);
  const Point<int> v (Math::round (v_float[0]), Math::round (v_float[1]), Math::round (v_float[2]));
  return Image::Nav::get_value_at_pos (voxel, v);

}





void Tck2nodes_radial::initialise_search ()
{

  const int max_axis_offset (Math::floor ((max_dist + max_add_dist) / minvalue (nodes.vox(0), nodes.vox(1), nodes.vox(2))));
  std::multimap< float, Point<int> > radial_search_map;
  Point<int> offset;
  for (offset[2] = -max_axis_offset; offset[2] <= +max_axis_offset; ++offset[2]) {
    for (offset[1] = -max_axis_offset; offset[1] <= +max_axis_offset; ++offset[1]) {
      for (offset[0] = -max_axis_offset; offset[0] <= +max_axis_offset; ++offset[0]) {
        const float dist = Math::sqrt (Math::pow2 (offset[2] * nodes.vox(2)) + Math::pow2 (offset[1] * nodes.vox(1)) + Math::pow2 (offset[0] * nodes.vox(0)));
        if (dist < (max_dist + max_add_dist))
          radial_search_map.insert (std::make_pair (dist, offset));
      }
    }
  }

  radial_search.reserve (radial_search_map.size());
  for (std::multimap<float, Point<int> >::const_iterator i = radial_search_map.begin(); i != radial_search_map.end(); ++i)
    radial_search.push_back (i->second);

}




node_t Tck2nodes_radial::select_node (const std::vector< Point<float> >& tck, VoxelType& voxel, bool end)
{

  float min_dist = max_dist;
  node_t node = 0;

  const Point<float>& p (end ? tck.back() : tck.front());
  const Point<float> v_float = transform.scanner2voxel (p);
  const Point<int> v (Math::round (v_float[0]), Math::round (v_float[1]), Math::round (v_float[2]));

  for (std::vector< Point<int> >::const_iterator offset = radial_search.begin(); offset != radial_search.end(); ++offset) {

    const Point<int> this_voxel (v + *offset);
    const Point<float> p_voxel (transform.voxel2scanner (this_voxel));
    const float dist ((p - p_voxel).norm());

    if (dist > max_dist + max_add_dist)
      return node;

    if (dist < min_dist && Image::Nav::within_bounds (voxel, this_voxel)) {
      const node_t this_node = Image::Nav::get_value_at_pos (voxel, this_voxel);
      if (this_node) {
        node = this_node;
        min_dist = dist;
      }
    }
  }
  return node;

}





node_t Tck2nodes_revsearch::select_node (const std::vector< Point<float> >& tck, VoxelType& voxel, bool end)
{

  const int midpoint_index = end ? (tck.size() / 2) : ((tck.size() + 1) / 2);
  const int start_index    = end ? (tck.size() - 1) : 0;
  const int step           = end ? -1 : 1;

  float dist = 0.0;

  for (int index = start_index; index != midpoint_index; index += step) {
    const Point<float>& p (tck[index]);
    const Point<float> v_float = transform.scanner2voxel (p);
    const Point<int> v (Math::round (v_float[0]), Math::round (v_float[1]), Math::round (v_float[2]));
    if (Image::Nav::within_bounds (voxel, v)) {
      const node_t this_node = Image::Nav::get_value_at_pos (voxel, v);
      if (this_node)
        return this_node;
    }
    if (max_dist && ((dist += (tck[index] - tck[index+step]).norm()) > max_dist))
      return 0;
  }

  return 0;

}





}
}
}
}



