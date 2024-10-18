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

#include "algo/loop.h"
#include "transform.h"

namespace MR::Registration::Warp {

template <class ImageType> void displacement2deformation(ImageType &input, ImageType &output) {
  MR::Transform transform(input);
  auto kernel = [&](ImageType &input, ImageType &output) {
    Eigen::Vector3d voxel((default_type)input.index(0), (default_type)input.index(1), (default_type)input.index(2));
    output.row(3) = (transform.voxel2scanner * voxel).template cast<typename ImageType::value_type>() +
                    Eigen::Vector3d(input.row(3));
  };
  ThreadedLoop(input, 0, 3).run(kernel, input, output);
}

template <class ImageType> void deformation2displacement(ImageType &input, ImageType &output) {
  MR::Transform transform(input);
  auto kernel = [&](ImageType &input, ImageType &output) {
    Eigen::Vector3d voxel((default_type)input.index(0), (default_type)input.index(1), (default_type)input.index(2));
    output.row(3) = Eigen::Vector3d(input.row(3)) - transform.voxel2scanner * voxel;
  };
  ThreadedLoop(input, 0, 3).run(kernel, input, output);
}
} // namespace MR::Registration::Warp
