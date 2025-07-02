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

#include "denoise/kernel/data.h"
#include "denoise/kernel/voxel.h"
#include "header.h"
#include "image.h"
#include "transform.h"

namespace MR::Denoise::Kernel {

class Base {
public:
  Base(const Header &H, const std::array<ssize_t, 3> &subsample_factors)
      : H(H),
        transform(H),
        halfvoxel_offsets({subsample_factors[0] & 1 ? 0.0 : 0.5,
                           subsample_factors[1] & 1 ? 0.0 : 0.5,
                           subsample_factors[2] & 1 ? 0.0 : 0.5}) {}
  Base(const Base &) = default;
  virtual ~Base() = default;

  // This can't be set during construction as it requires having first loaded the image,
  //   and that is templated; so this needs to happen afterwards
  void set_mask(Image<bool> &in) { mask_image = in; }

  // This is just for pre-allocating matrices
  virtual ssize_t estimated_size() const = 0;
  // This is the interface that kernels must provide
  virtual Data operator()(const Voxel::index_type &) const = 0;

protected:
  const Header H;
  const Transform transform;
  std::array<default_type, 3> halfvoxel_offsets;

  Image<bool> mask_image;

  // For translating the index of a processed voxel
  //   into a realspace position corresponding to the centre of the patch,
  //   accounting for the fact that subsampling may be introducing an offset
  //   such that the actual centre of the patch is not at the centre of this voxel
  Eigen::Vector3d voxel2real(const Kernel::Voxel::index_type &pos) const {
    return (                                               //
        transform.voxel2scanner *                          //
        Eigen::Vector3d({pos[0] + halfvoxel_offsets[0],    //
                         pos[1] + halfvoxel_offsets[1],    //
                         pos[2] + halfvoxel_offsets[2]})); //
  }
};

} // namespace MR::Denoise::Kernel
