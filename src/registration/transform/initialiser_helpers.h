/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#ifndef __registration_transform_initialiser_helpers_h__
#define __registration_transform_initialiser_helpers_h__

#include <algorithm>
#include <Eigen/Geometry>
#include "image.h"
#include "transform.h"
#include "math/math.h"
#include "math/SH.h"
#include "registration/transform/base.h"
#include "registration/multi_contrast.h"

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
            Eigen::Vector3d centre_voxel;
            centre_voxel[0] = (static_cast<default_type>(image.size(0)) / 2.0) - 1.0;
            centre_voxel[1] = (static_cast<default_type>(image.size(1)) / 2.0) - 1.0;
            centre_voxel[2] = (static_cast<default_type>(image.size(2)) / 2.0) - 1.0;
            MR::Transform transform (image);
            centre = transform.voxel2scanner * centre_voxel;
          }

        void get_centre_of_mass (Image<default_type>& im,
                                 Image<default_type>& mask,
                                 Eigen::Vector3d& centre_of_mass,
                                 const vector<MultiContrastSetting>& contrast_settings);

        bool get_sorted_eigen_vecs_vals (const Eigen::Matrix<default_type, 3, 3>& mat,
          Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>& eigenvectors,
          Eigen::Matrix<default_type, Eigen::Dynamic, 1>& eigenvals);

        class FODInitialiser { MEMALIGN(FODInitialiser)
          public:
            FODInitialiser (Image<default_type>& image1,
                                Image<default_type>& image2,
                                Image<default_type>& mask1,
                                Image<default_type>& mask2,
                                Registration::Transform::Base& transform,
                                ssize_t l_max = -1):
              im1(image1),
              im2(image2),
              transform(transform),
              mask1(mask1),
              mask2(mask2),
              lmax (l_max) {
                assert (im1.ndim() == 4 && im2.ndim() == 4);
                assert (im1.size(3) == im2.size(3));
                ssize_t l = Math::SH::LforN(im1.size(3));
                if (lmax == -1 or lmax > l) lmax = l;
                N = Math::SH::NforL(lmax);
                sh1.resize(N);
                sh1.setZero();
                sh2.resize(N);
                sh2.setZero();
              }

            void run ();


          protected:
            void init (Image<default_type>& image,
              Image<default_type>& mask,
              Eigen::Matrix<default_type, Eigen::Dynamic, 1>& sh,
              Eigen::Matrix<default_type, 3, 1>& centre_of_mass);

          private:
            Image<default_type>& im1;
            Image<default_type>& im2;
            Registration::Transform::Base& transform;
            Image<default_type>& mask1;
            Image<default_type>& mask2;
            ssize_t lmax, N;
            Eigen::Matrix<default_type, 3, 1> im1_centre_of_mass, im2_centre_of_mass;
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> sh1, sh2;
        };

        class MomentsInitialiser { MEMALIGN(MomentsInitialiser)
          public:
            MomentsInitialiser (Image<default_type>& image1,
                                Image<default_type>& image2,
                                Image<default_type>& mask1,
                                Image<default_type>& mask2,
                                Registration::Transform::Base& transform,
                                const vector<MultiContrastSetting>& contrast):
              im1 (image1),
              im2 (image2),
              transform (transform),
              mask1 (mask1),
              mask2 (mask2),
              contrast_settings (contrast) { }

            void run ();


          protected:
            bool calculate_eigenvectors (
              Image<default_type>& image_1,
              Image<default_type>& image_2,
              Image<default_type>& mask_1,
              Image<default_type>& mask_2);
            void create_moments_images ();

          private:
            Image<default_type>& im1;
            Image<default_type>& im2;
            Registration::Transform::Base& transform;
            Image<default_type>& mask1;
            Image<default_type>& mask2;
            const vector<MultiContrastSetting>& contrast_settings;
            Eigen::Vector3d im1_centre, im2_centre;
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
