/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include "denoise/kernel/sphere_ratio.h"

namespace MR::Denoise::Kernel {

Data SphereRatio::operator()(const Voxel::index_type &pos) const {
  assert(mask_image.valid());
  // For thread-safety
  Image<bool> mask(mask_image);
  Data result(voxel2real(pos), centre_index);
  auto table_it = shared->begin();
  while (table_it != shared->end()) {
    // If there's a tie in distances, want to include all such offsets in the kernel,
    //   even if the size of the utilised kernel extends beyond the minimum size
    if (result.voxels.size() >= min_size && table_it->sq_distance != result.max_distance)
      break;
    const Voxel::index_type voxel({pos[0] + table_it->index[0],   //
                                   pos[1] + table_it->index[1],   //
                                   pos[2] + table_it->index[2]}); //
    if (!is_out_of_bounds(H, voxel, 0, 3)) {
      assign_pos_of(voxel).to(mask);
      if (mask.value()) {
        result.voxels.push_back(Voxel(voxel, table_it->sq_distance));
        result.max_distance = table_it->sq_distance;
      }
    }
    ++table_it;
  }
  result.max_distance = std::sqrt(result.max_distance);
  return result;
}

default_type SphereRatio::compute_max_radius(const Header &voxel_grid, const default_type min_ratio) const {
  size_t values_per_voxel = voxel_grid.size(3);
  for (size_t axis = 4; axis != voxel_grid.ndim(); ++axis)
    values_per_voxel *= voxel_grid.size(axis);
  const default_type voxel_volume = voxel_grid.spacing(0) * voxel_grid.spacing(1) * voxel_grid.spacing(2);
  // Consider the worst case scenario, where the corner of the FoV is being processed;
  //   we do not want to run out of elements in our lookup table before reaching our desired # voxels
  // Define a sphere for which the volume is eight times that of what would be required
  //   for processing a voxel in the middle of the FoV;
  //   only one of the eight octants will have valid image data
  const default_type sphere_volume = 8.0 * values_per_voxel * min_ratio * voxel_volume;
  const default_type approx_radius = std::sqrt(sphere_volume * 0.75 / Math::pi);
  const Voxel::index_type half_extents({ssize_t(std::ceil(approx_radius / voxel_grid.spacing(0))),   //
                                        ssize_t(std::ceil(approx_radius / voxel_grid.spacing(1))),   //
                                        ssize_t(std::ceil(approx_radius / voxel_grid.spacing(2)))}); //
  default_type max_radius = std::max({half_extents[0] * voxel_grid.spacing(0),                       //
                                      half_extents[1] * voxel_grid.spacing(1),                       //
                                      half_extents[2] * voxel_grid.spacing(2)});                     //
  DEBUG("Calibrating maximal radius for building aspect ratio spherical denoising kernel:");
  std::string dim_string = str(voxel_grid.size(0));
  for (size_t axis = 1; axis != voxel_grid.ndim(); ++axis)
    dim_string += "x" + str(voxel_grid.size(axis));
  DEBUG("  Image dimensions: " + dim_string);
  DEBUG("  Values per voxel: " + str(values_per_voxel));
  std::string spacing_string = str(voxel_grid.spacing(0));
  for (size_t axis = 1; axis != 3; ++axis)
    spacing_string += "x" + str(voxel_grid.spacing(axis));
  DEBUG("  Voxel spacing: " + spacing_string + "mm");
  DEBUG("  Voxel volume: " + str(voxel_volume) + "mm^3");
  DEBUG("  Maximal sphere volume: " + str(sphere_volume));
  DEBUG("  Approximate radius: " + str(approx_radius));
  std::string halfextent_string = str(half_extents[0]);
  for (size_t axis = 1; axis != 3; ++axis)
    halfextent_string += "," + str(half_extents[axis]);
  DEBUG("  Half-extents: [" + halfextent_string + "]");
  std::string maxradius_string = str(half_extents[0] * voxel_grid.spacing(0));
  for (size_t axis = 1; axis != 3; ++axis)
    maxradius_string += "," + str(half_extents[axis] * voxel_grid.spacing(axis));
  DEBUG("  Maximum radius = max(" + maxradius_string + ") = " + str(max_radius));
  return max_radius;
}

} // namespace MR::Denoise::Kernel
