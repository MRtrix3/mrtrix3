/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __math_average_space_h__
#define __math_average_space_h__

#include <unsupported/Eigen/MatrixFunctions>
#include <Eigen/SVD>
#include <Eigen/Geometry>
#include "transform.h"
#include "image.h"
#include "debug.h"


namespace MR
{
   namespace Math
   {
      double matrix_average (vector<Eigen::MatrixXd> const &mat_in, Eigen::MatrixXd& mat_avg, bool verbose = false);
   }
 }

 namespace MR {

  Eigen::Matrix<default_type, 8, 4> get_cuboid_corners (const Eigen::Matrix<default_type, 4, 1>& xzx1);
  Eigen::Matrix<default_type, 8, 4> get_bounding_box (const Header& header, const Eigen::Transform<default_type, 3, Eigen::Projective>& voxel2scanner);

  Header compute_minimum_average_header (const vector<Header>& input_headers,
                                         int voxel_subsampling,
                                         const Eigen::Matrix<default_type, 4, 1>& padding,
                                         const vector<Eigen::Transform<default_type, 3, Eigen::Projective>>& transform_header_with);

  template<class ImageType1, class ImageType2, class TransformationType>
    Header compute_minimum_average_header (
        const ImageType1& im1,
        const ImageType2& im2,
        const TransformationType& transformation,
        const default_type& voxel_subsampling,
        const Eigen::Matrix<default_type, 4, 1>& padding) {
      vector<Eigen::Transform<default_type, 3, Eigen::Projective>> init_transforms;
      Eigen::Transform<default_type, 3, Eigen::Projective> trafo_1 = transformation.get_transform_half_inverse();
      Eigen::Transform<default_type, 3, Eigen::Projective> trafo_2 = transformation.get_transform_half();
      init_transforms.push_back (trafo_1);
      init_transforms.push_back (trafo_2);
      vector<Header> headers;
      headers.push_back(Header(im1));
      headers.push_back(Header(im2));
      return compute_minimum_average_header(headers, voxel_subsampling, padding, init_transforms);
    }
 }
#endif
