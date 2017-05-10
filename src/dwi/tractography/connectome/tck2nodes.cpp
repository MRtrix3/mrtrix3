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


#include <map>
#include <set>

#include "dwi/tractography/connectome/tck2nodes.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {





node_t Tck2nodes_end_voxels::select_node (const Tractography::Streamline<>& tck, Image<node_t>& v, const bool end) const
{
  const Eigen::Vector3 p ((end ? tck.back() : tck.front()).cast<default_type>());
  const Eigen::Vector3 v_float (transform->scanner2voxel * p);
  for (size_t axis = 0; axis != 3; ++axis)
    v.index(axis) = std::round (v_float[axis]);
  if (is_out_of_bounds (v))
    return 0;
  else
    return v.value();
}





void Tck2nodes_radial::initialise_search ()
{
  const int max_axis_offset (std::ceil ((max_dist + max_add_dist) / std::min ( { nodes.spacing(0), nodes.spacing(1), nodes.spacing(2) } )));
  std::multimap<default_type, voxel_type> radial_search_map;
  voxel_type offset;
  for (offset[2] = -max_axis_offset; offset[2] <= +max_axis_offset; ++offset[2]) {
    for (offset[1] = -max_axis_offset; offset[1] <= +max_axis_offset; ++offset[1]) {
      for (offset[0] = -max_axis_offset; offset[0] <= +max_axis_offset; ++offset[0]) {
        const default_type dist = std::sqrt (Math::pow2 (offset[2] * nodes.spacing(2)) + Math::pow2 (offset[1] * nodes.spacing(1)) + Math::pow2 (offset[0] * nodes.spacing(0)));
        if (dist < (max_dist + max_add_dist))
          radial_search_map.insert (std::make_pair (dist, offset));
      }
    }
  }
  radial_search.reserve (radial_search_map.size());
  for (auto i = radial_search_map.begin(); i != radial_search_map.end(); ++i)
    radial_search.push_back (i->second);
}




node_t Tck2nodes_radial::select_node (const Tractography::Streamline<>& tck, Image<node_t>& v, const bool end) const
{
  default_type min_dist = max_dist;
  node_t node = 0;

  const Eigen::Vector3 p = (end ? tck.back() : tck.front()).cast<default_type>();
  const Eigen::Vector3 v_float = transform->scanner2voxel * p;
  const voxel_type centre { int(std::round (v_float[0])), int(std::round (v_float[1])), int(std::round (v_float[2])) };

  for (vector<voxel_type>::const_iterator offset = radial_search.begin(); offset != radial_search.end(); ++offset) {

    const voxel_type this_voxel (centre + *offset);
    const Eigen::Vector3 p_voxel (transform->voxel2scanner * this_voxel.matrix().cast<default_type>());
    const default_type dist ((p - p_voxel).norm());

    if (dist > min_dist + 2*max_add_dist)
      return node;

    if (dist < min_dist) {
      assign_pos_of (this_voxel).to (v);
      if (!is_out_of_bounds (v)) {
        const node_t this_node = v.value();
        if (this_node) {
          node = this_node;
          min_dist = dist;
        }
      }
    }
  }
  return node;
}





node_t Tck2nodes_revsearch::select_node (const Tractography::Streamline<>& tck, Image<node_t>& v, const bool end) const
{
  const int midpoint_index = end ? (tck.size() / 2) : ((tck.size() + 1) / 2);
  const int start_index    = end ? (tck.size() - 1) : 0;
  const int step           = end ? -1 : 1;

  default_type dist = 0.0;

  for (int index = start_index; index != midpoint_index; index += step) {
    const Eigen::Vector3 v_float = transform->scanner2voxel * tck[index].cast<default_type>();
    const voxel_type voxel { int(std::round (v_float[0])), int(std::round (v_float[1])), int(std::round (v_float[2])) };
    assign_pos_of (voxel).to (v);
    if (!is_out_of_bounds (v)) {
      const node_t this_node = v.value();
      if (this_node)
        return this_node;
    }
    if (max_dist && ((dist += (tck[index] - tck[index+step]).norm()) > max_dist))
      return 0;
  }

  return 0;
}






node_t Tck2nodes_forwardsearch::select_node (const Tractography::Streamline<>& tck, Image<node_t>& v, const bool end) const
{
  // Start by defining the endpoint and the tangent at the endpoint
  const int index = end ? (tck.size() - 1) : 0;
  const int step  = end ? -1 : 1;
  const Eigen::Vector3 p = tck[index].cast<default_type>();
  Eigen::Vector3 t;

  // Heuristic for determining the tangent at the streamline endpoint
  if (tck.size() > 2) {
    const Eigen::Vector3 second_last_step = (tck[index+step] - tck[index+2*step]).cast<default_type>();
    const Eigen::Vector3 last_step = (tck[index] - tck[index+step]).cast<default_type>();
    const default_type length_ratio = last_step.norm() / second_last_step.norm();
    const Eigen::Vector3 curvature (last_step - (second_last_step * length_ratio));
    t = last_step + curvature;
  } else {
    t = (tck[index] - tck[index+step]).cast<default_type>();
  }
  t.normalize();

  // Need to store a list of those voxels that have already been considered
  std::set<voxel_type> visited;
  // Also a list of voxels to test & potentially expand - based on cost function
  std::map<default_type, voxel_type> to_test;

  // Voxel containing streamline endpoint not guaranteed to be appropriate
  // Should it be tested anyway? Probably
  const Eigen::Vector3 vp (transform->scanner2voxel * p);
  const voxel_type voxel { int(std::round (vp[0])), int(std::round (vp[1])), int(std::round (vp[2])) };
  if (is_out_of_bounds (v, voxel))
    return 0;
  visited.insert (voxel);
  to_test.insert (std::make_pair (default_type(0.0), voxel));

  while (to_test.size()) {

    const voxel_type voxel = to_test.begin()->second;
    to_test.erase (to_test.begin());

    assign_pos_of (voxel).to (v);
    if (is_out_of_bounds (v))
      continue;
    const node_t value = v.value();
    if (value)
      return value;

    // Check voxel neighbours
    voxel_type offset;
    for (offset[2] = -1; offset[2] <= 1; ++offset[2]) {
      for (offset[1] = -1; offset[1] <= 1; ++offset[1]) {
        for (offset[0] = -1; offset[0] <= 1; ++offset[0]) {

          const voxel_type v_neighbour = voxel + offset;
          if (visited.find (v_neighbour) == visited.end()) {
            visited.insert (v_neighbour);
            const float cf = get_cf (p, t, v_neighbour);
            if (std::isfinite (cf))
              to_test.insert (std::make_pair (cf, v_neighbour));
          }

        }
      }
    }

  }

  return 0;
}



default_type Tck2nodes_forwardsearch::get_cf (const Eigen::Vector3& p, const Eigen::Vector3& t, const voxel_type& v) const
{
  const Eigen::Vector3 vfloat { default_type(v[0]), default_type(v[1]), default_type(v[2]) };
  const Eigen::Vector3 vp (transform->voxel2scanner * vfloat);
  Eigen::Vector3 offset (vp - p);
  const default_type dist = offset.norm();
  offset.normalize();
  const default_type angle = std::acos (t.dot (offset));
  if (angle > angle_limit)
    return NAN;

  // Multiplier of 1 for straight ahead, 2 for right at the angle limit
  // Makes the search space a kind of diamond shape
  const default_type multiplier = 1.0 + (angle / angle_limit);
  const default_type cf = dist * multiplier;
  return (cf > max_dist ? NAN : cf);
}










void Tck2nodes_all_voxels::select_nodes (const Streamline<>& tck, Image<node_t>& v, vector<node_t>& out) const
{
  std::set<node_t> result;
  for (Streamline<>::const_iterator p = tck.begin(); p != tck.end(); ++p) {
    const Eigen::Vector3 v_float = transform->scanner2voxel * p->cast<default_type>();
    const voxel_type voxel (int(std::round (v_float[0])), int(std::round (v_float[1])), int(std::round (v_float[2])));
    assign_pos_of (voxel).to (v);
    if (!is_out_of_bounds (v)) {
      const node_t this_node = v.value();
      if (this_node)
        result.insert (this_node);
    }
  }
  out.clear();
  for (std::set<node_t>::const_iterator n = result.begin(); n != result.end(); ++n)
    out.push_back (*n);
}







}
}
}
}



