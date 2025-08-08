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

#pragma once

#include <Eigen/Dense>

#include "types.h"

namespace MR::Denoise::Kernel {

template <class T> class VoxelBase {
public:
  using index_type = Eigen::Array<T, 3, 1>;
  VoxelBase(const index_type &index, const default_type sq_distance) : index(index), sq_distance(sq_distance) {}
  VoxelBase(const VoxelBase &) = default;
  VoxelBase(VoxelBase &&) = default;
  ~VoxelBase() {}
  VoxelBase &operator=(const VoxelBase &that) {
    index = that.index;
    sq_distance = that.sq_distance;
    return *this;
  }
  VoxelBase &operator=(VoxelBase &&that) noexcept {
    index = that.index;
    sq_distance = that.sq_distance;
    return *this;
  }
  bool operator<(const VoxelBase &that) const { return sq_distance < that.sq_distance; }
  default_type distance() const { return std::sqrt(sq_distance); }

  index_type index;
  default_type sq_distance;
};

// Need signed integer to represent offsets from the centre of the kernel;
//   however absolute voxel indices should be unsigned
// For nonstationarity correction,
//   "Voxel" also needs a noise level estimate per voxel in the patch,
//   as the scaling on insertion of data into the matrix
//   needs to be reversed when reconstructing the denoised signal from the eigenvectors
class Voxel : public VoxelBase<ssize_t> {
public:
  using index_type = VoxelBase<ssize_t>::index_type;
  Voxel(const index_type &index, const default_type sq_distance, const default_type noise_level)
      : VoxelBase<ssize_t>(index, sq_distance), noise_level(noise_level) {}
  Voxel(const index_type &index, const default_type sq_distance)
      : VoxelBase<ssize_t>(index, sq_distance), noise_level(std::numeric_limits<default_type>::signaling_NaN()) {}
  default_type noise_level;
};

using Offset = VoxelBase<int>;

} // namespace MR::Denoise::Kernel
