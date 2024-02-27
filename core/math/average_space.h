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

#ifndef __math_average_space_h__
#define __math_average_space_h__

#include "debug.h"
#include "image.h"
#include "transform.h"
#include <Eigen/Geometry>
#include <Eigen/SVD>
#include <unsupported/Eigen/MatrixFunctions>

namespace MR::Math {
double matrix_average(std::vector<Eigen::MatrixXd> const &mat_in, Eigen::MatrixXd &mat_avg, bool verbose = false);
}

namespace MR {

Eigen::Matrix<default_type, 8, 4> get_cuboid_corners(const Eigen::Matrix<default_type, 4, 1> &xzx1);
Eigen::Matrix<default_type, 8, 4>
get_bounding_box(const Header &header, const Eigen::Transform<default_type, 3, Eigen::Projective> &voxel2scanner);

Header compute_minimum_average_header(
    const std::vector<Header> &input_headers,
    const std::vector<Eigen::Transform<default_type, 3, Eigen::Projective>> &transform_header_with,
    int voxel_subsampling = 1,
    Eigen::Matrix<default_type, 4, 1> padding = Eigen::Matrix<default_type, 4, 1>(1.0, 1.0, 1.0, 1.0));

template <class ImageType1, class ImageType2>
Header compute_minimum_average_header(
    const ImageType1 &im1,
    const ImageType2 &im2,
    Eigen::Transform<default_type, 3, Eigen::Projective> transform_1 =
        Eigen::Transform<default_type, 3, Eigen::Projective>::Identity(),
    Eigen::Transform<default_type, 3, Eigen::Projective> transform_2 =
        Eigen::Transform<default_type, 3, Eigen::Projective>::Identity(),
    Eigen::Matrix<default_type, 4, 1> padding = Eigen::Matrix<default_type, 4, 1>(1.0, 1.0, 1.0, 1.0),
    int voxel_subsampling = 1) {
  std::vector<Eigen::Transform<default_type, 3, Eigen::Projective>> init_transforms{transform_1, transform_2};
  std::vector<Header> headers{im1, im2};
  return compute_minimum_average_header(headers, init_transforms, voxel_subsampling, padding);
}
} // namespace MR
#endif
