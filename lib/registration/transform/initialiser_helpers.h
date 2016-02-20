/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */

#ifndef __registration_transform_initialiser_helpers_h__
#define __registration_transform_initialiser_helpers_h__

#include <algorithm>
#include "image.h"
#include "registration/transform/base.h"
#include "transform.h"

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {
      namespace Init
      {
        template <class ImageType, class ValueType>
          void get_geometric_centre (const ImageType& image, Eigen::Matrix<ValueType, 3, 1>& centre)
          {
            Eigen::Vector3 centre_voxel;
            centre_voxel[0] = (static_cast<default_type>(image.size(0)) / 2.0) - 1.0;
            centre_voxel[1] = (static_cast<default_type>(image.size(1)) / 2.0) - 1.0;
            centre_voxel[2] = (static_cast<default_type>(image.size(2)) / 2.0) - 1.0;
            MR::Transform transform (image);
            centre  = transform.voxel2scanner * centre_voxel;
          }

        bool get_sorted_eigen_vecs_vals (const Eigen::Matrix<default_type, 3, 3>& mat,
          Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>& eigenvectors,
          Eigen::Matrix<default_type, Eigen::Dynamic, 1>& eigenvals);

        void get_moments (Image<default_type>& image,
                          Image<default_type>& mask,
                          const Eigen::Matrix<default_type, 3, 1>& centre,
                          default_type& m000,
                          default_type& m100,
                          default_type& m010,
                          default_type& m001,
                          default_type& mu110,
                          default_type& mu011,
                          default_type& mu101,
                          default_type& mu200,
                          default_type& mu020,
                          default_type& mu002);

        class MomentsInitialiser {
          public:
            MomentsInitialiser (Image<default_type>& image1,
                                Image<default_type>& image2,
                                Image<default_type>& mask1,
                                Image<default_type>& mask2,
                                Registration::Transform::Base& transform):
              im1(image1),
              im2(image2),
              transform(transform),
              mask1(mask1),
              mask2(mask2) {};

            void run ();
          protected:
            bool calculate_eigenvectors ();

          private:
            Image<default_type>& im1;
            Image<default_type>& im2;
            Registration::Transform::Base& transform;
            Image<default_type>& mask1;
            Image<default_type>& mask2;
            Eigen::Vector3 im1_centre, im2_centre;
            Eigen::Matrix<default_type, 3, 1> im1_centre_of_mass, im2_centre_of_mass;
            Eigen::Matrix<default_type, 3, 3> im1_covariance_matrix, im2_covariance_matrix;
            Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> im1_evec, im2_evec;
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> im1_eval, im2_eval;
        };



      }
    }
  }
}

#endif
