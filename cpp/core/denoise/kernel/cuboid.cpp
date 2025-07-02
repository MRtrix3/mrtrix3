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

#include "denoise/kernel/cuboid.h"

namespace MR::Denoise::Kernel {

Cuboid::Cuboid(const Header &header,
               const std::array<ssize_t, 3> &subsample_factors,
               const std::array<ssize_t, 3> &extent)
    : Base(header, subsample_factors),
      size(extent[0] * extent[1] * extent[2]),
      // Only sensible if no subsampling is performed,
      //   and every single DWI voxel is reconstructed from a patch centred at that voxel,
      //   with no overcomplete local PCA (aggregator == EXCLUSIVE)
      centre_index(subsample_factors == std::array<ssize_t, 3>({1, 1, 1}) ? (size / 2) : -1) {
  for (ssize_t axis = 0; axis != 3; ++axis) {
    if (subsample_factors[axis] % 2) {
      if (!(extent[axis] % 2))
        throw Exception("For no subsampling / subsampling by an odd number, "
                        "size of cubic kernel must be an odd integer");
      bounding_box(axis, 0) = -extent[axis] / 2;
      bounding_box(axis, 1) = extent[axis] / 2;
    } else {
      if (extent[axis] % 2)
        throw Exception("For subsampling by an even number, "
                        "size of cubic kernel must be an even integer");
      bounding_box(axis, 0) = 1 - extent[axis] / 2;
      bounding_box(axis, 1) = extent[axis] / 2;
    }
  }
}

namespace {
// patch handling at image edges
inline ssize_t wrapindex(int p, int r, int bbminus, int bbplus, int max) {
  int rr = p + r;
  if (rr < 0)
    rr = bbplus - r;
  if (rr >= max)
    rr = (max - 1) + bbminus - r;
  return rr;
}
} // namespace

Data Cuboid::operator()(const Voxel::index_type &pos) const {
  assert(mask_image.valid());
  // For thread-safety
  Image<bool> mask(mask_image);
  Data result(voxel2real(pos), centre_index);
  Voxel::index_type voxel;
  Offset::index_type offset;
  for (offset[2] = bounding_box(2, 0); offset[2] <= bounding_box(2, 1); ++offset[2]) {
    voxel[2] = wrapindex(pos[2], offset[2], bounding_box(2, 0), bounding_box(2, 1), H.size(2));
    for (offset[1] = bounding_box(1, 0); offset[1] <= bounding_box(1, 1); ++offset[1]) {
      voxel[1] = wrapindex(pos[1], offset[1], bounding_box(1, 0), bounding_box(1, 1), H.size(1));
      for (offset[0] = bounding_box(0, 0); offset[0] <= bounding_box(0, 1); ++offset[0]) {
        voxel[0] = wrapindex(pos[0], offset[0], bounding_box(0, 0), bounding_box(0, 1), H.size(0));
        assert(!is_out_of_bounds(H, voxel, 0, 3));
        assign_pos_of(voxel).to(mask);
        if (mask.value()) {
          const default_type sq_distance = Math::pow2(pos[0] + halfvoxel_offsets[0] - voxel[0]) * H.spacing(0) + //
                                           Math::pow2(pos[1] + halfvoxel_offsets[1] - voxel[1]) * H.spacing(1) + //
                                           Math::pow2(pos[2] + halfvoxel_offsets[2] - voxel[2]) * H.spacing(2);  //
          result.voxels.push_back(Voxel(voxel, sq_distance));
          result.max_distance = std::max(result.max_distance, sq_distance);
        }
      }
    }
  }
  result.max_distance = std::sqrt(result.max_distance);
  return result;
}

} // namespace MR::Denoise::Kernel
