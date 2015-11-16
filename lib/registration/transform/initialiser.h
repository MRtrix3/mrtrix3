/*
 Copyright 2012 Brain Research Institute, Melbourne, Australia

 Written by David Raffelt, 23/02/2012.

 This file is part of MRtrix.

 MRtrix is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 MRtrix is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */


#ifndef __registration_transform_initialiser_h__
#define __registration_transform_initialiser_h__

#include "transform.h"
#include "algo/loop.h"
#include <Eigen/Eigenvalues>

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {
      namespace Init
      {
        enum InitType {mass, geometric, moments, none};

        template <class Im1ImageType,
                  class Im2ImageType,
                  class TransformType>
          void initialise_using_image_centres (const Im1ImageType& im1,
                                               const Im2ImageType& im2,
                                               TransformType& transform)
          {
            CONSOLE ("initialising centre of rotation and translation using geometric centre");
            Eigen::Vector3 im1_centre_voxel;
            im1_centre_voxel[0] = (static_cast<default_type>(im1.size(0)) / 2.0) - 1.0;
            im1_centre_voxel[1] = (static_cast<default_type>(im1.size(1)) / 2.0) - 1.0;
            im1_centre_voxel[2] = (static_cast<default_type>(im1.size(2)) / 2.0) - 1.0;
            MR::Transform im1_transform (im1);
            Eigen::Vector3 im1_centre_scanner  = im1_transform.voxel2scanner * im1_centre_voxel;

            Eigen::Vector3 im2_centre_voxel;
            im2_centre_voxel[0] = (static_cast<default_type>(im2.size(0)) / 2.0) - 1.0;
            im2_centre_voxel[1] = (static_cast<default_type>(im2.size(1)) / 2.0) - 1.0;
            im2_centre_voxel[2] = (static_cast<default_type>(im2.size(2)) / 2.0) - 1.0;
            MR::Transform im2_transform (im2);
            Eigen::Vector3 im2_centre_scanner = im2_transform.voxel2scanner * im2_centre_voxel;

            Eigen::Vector3 translation = im1_centre_scanner - im2_centre_scanner;
            Eigen::Vector3 centre = (im1_centre_scanner + im2_centre_scanner) / 2.0;
            transform.set_centre (centre);
            transform.set_translation (translation);
          }

        template <class ImageType, class ValueType>
          void get_moments (ImageType& image,
                            Eigen::Matrix<ValueType, 3, 1>& centre,
                            ValueType& m000,
                            ValueType& m100,
                            ValueType& m010,
                            ValueType& m001,
                            ValueType& mu110,
                            ValueType& mu011,
                            ValueType& mu101,
                            ValueType& mu200,
                            ValueType& mu020,
                            ValueType& mu002)
          {
            centre.setZero();
            m000 = 0.0;
            m100 = 0.0;
            m010 = 0.0;
            m001 = 0.0;
            mu110 = 0.0;
            mu011 = 0.0;
            mu101 = 0.0;
            mu200 = 0.0;
            mu020 = 0.0;
            mu002 = 0.0;

            Eigen::Vector3 centre_voxel;
            centre_voxel[0] = (static_cast<default_type>(image.size(0)) / 2.0) - 1.0;
            centre_voxel[1] = (static_cast<default_type>(image.size(1)) / 2.0) - 1.0;
            centre_voxel[2] = (static_cast<default_type>(image.size(2)) / 2.0) - 1.0;
            MR::Transform transform (image);
            centre  = transform.voxel2scanner * centre_voxel;

            // only use the first volume of a 4D file. This is important for FOD images.
            for (auto i = Loop (0, 3)(image); i; ++i) {
              // Eigen::Vector3 voxel_pos ((default_type)image.index(0), (default_type)image.index(1), (default_type)image.index(2));
              // Eigen::Vector3 scanner_pos = im1_transform.voxel2scanner * voxel_pos;
              m000 += image.value();
            }
            // only use the first volume of a 4D file. This is important for FOD images.
            for (auto i = Loop (0, 3)(image); i; ++i) {
              Eigen::Vector3 voxel_pos ((default_type)image.index(0), (default_type)image.index(1), (default_type)image.index(2));
              Eigen::Vector3 scanner_pos = transform.voxel2scanner * voxel_pos;
              ValueType val = image.value();

              ValueType xc = scanner_pos[0] - centre[0];
              ValueType yc = scanner_pos[1] - centre[1];
              ValueType zc = scanner_pos[2] - centre[2];

              m100 += scanner_pos[0] * val;
              m010 += scanner_pos[1] * val;
              m001 += scanner_pos[2] * val;

              mu110 += xc * yc * val;
              mu011 += yc * zc * val;
              mu101 += xc * zc * val;

              mu200 += Math::pow2(xc) * val;
              mu020 += Math::pow2(yc) * val;
              mu002 += Math::pow2(zc) * val;
            }
          }

        template <class Im1ImageType,
                  class Im2ImageType,
                  class TransformType>
          void initialise_using_image_moments (Im1ImageType& im1,
                                               Im2ImageType& im2,
                                               TransformType& transform)
          {
            CONSOLE ("initialising using image moments");
            Eigen::Vector3 im1_centre;
            default_type  m000, m100, m010, m001, mu110, mu011, mu101, mu200, mu020, mu002;

            get_moments (im1, im1_centre, m000, m100, m010, m001, mu110, mu011, mu101, mu200, mu020, mu002);
            Eigen::Matrix<default_type, 3, 3> im1_covariance_matrix;
            Eigen::Matrix<default_type, 3, 1> im1_centre_of_mass;
            im1_centre_of_mass << m100 / m000, m010 / m000, m001 / m000;
            im1_covariance_matrix(0, 0) = mu200 / m000;
            im1_covariance_matrix(0, 1) = mu110 / m000;
            im1_covariance_matrix(0, 2) = mu101 / m000;
            im1_covariance_matrix(1, 0) = mu011 / m000;
            im1_covariance_matrix(1, 1) = mu020 / m000;
            im1_covariance_matrix(1, 2) = mu110 / m000;
            im1_covariance_matrix(2, 0) = mu101 / m000;
            im1_covariance_matrix(2, 1) = mu011 / m000;
            im1_covariance_matrix(2, 2) = mu002 / m000;
            Eigen::EigenSolver<Eigen::MatrixXd> es1(im1_covariance_matrix);
            auto im1_ev = es1.eigenvectors();

            Eigen::Vector3 im2_centre;
            get_moments (im2, im2_centre, m000, m100, m010, m001, mu110, mu011, mu101, mu200, mu020, mu002);
            Eigen::Matrix<default_type, 3, 3> im2_covariance_matrix;
            Eigen::Matrix<default_type, 3, 1> im2_centre_of_mass;
            im2_centre_of_mass << m100 / m000, m010 / m000, m001 / m000;

            im2_covariance_matrix(0, 0) = mu200 / m000;
            im2_covariance_matrix(0, 1) = mu110 / m000;
            im2_covariance_matrix(0, 2) = mu101 / m000;
            im2_covariance_matrix(1, 0) = mu011 / m000;
            im2_covariance_matrix(1, 1) = mu020 / m000;
            im2_covariance_matrix(1, 2) = mu110 / m000;
            im2_covariance_matrix(2, 0) = mu101 / m000;
            im2_covariance_matrix(2, 1) = mu011 / m000;
            im2_covariance_matrix(2, 2) = mu002 / m000;
            Eigen::EigenSolver<Eigen::MatrixXd> es2(im2_covariance_matrix);
            auto im2_ev = es2.eigenvectors();

            // auto eigenvals = covariance_matrix.eigenvalues();
            Eigen::Vector3 centre = (im1_centre_of_mass + im2_centre_of_mass) / 2.0;
            transform.set_centre (centre);
            Eigen::Vector3 translation = im1_centre_of_mass - im2_centre_of_mass;
            transform.set_translation (translation);
            VAR(centre);
            VAR(translation);

            // TODO init rotation
            VAR(im1_ev.col(0).transpose());
            VAR(im1_ev.col(1).transpose());
            VAR(im1_ev.col(2).transpose());

            VAR(im2_ev.col(0).transpose());
            VAR(im2_ev.col(1).transpose());
            VAR(im2_ev.col(2).transpose());
        }

        template <class Im1ImageType,
                  class Im2ImageType,
                  class TransformType>
          void initialise_using_image_mass (Im1ImageType& im1,
                                            Im2ImageType& im2,
                                            TransformType& transform)
          {
            CONSOLE ("initialising centre of rotation and translation using centre of mass");
            Eigen::Matrix<typename TransformType::ParameterType, 3, 1>  im1_centre_of_mass (3);
            im1_centre_of_mass.setZero();
            default_type im1_mass = 0.0;
            MR::Transform im1_transform (im1);
            // only use the first volume of a 4D file. This is important for FOD images.
            for (auto i = Loop (0, 3)(im1); i; ++i) {
              Eigen::Vector3 voxel_pos ((default_type)im1.index(0), (default_type)im1.index(1), (default_type)im1.index(2));
              Eigen::Vector3 im1_scanner = im1_transform.voxel2scanner * voxel_pos;
              im1_mass += im1.value();
              im1_centre_of_mass += im1_scanner * im1.value();
            }
            im1_centre_of_mass /= im1_mass;

            Eigen::Matrix<typename TransformType::ParameterType, 3, 1> im2_centre_of_mass (3);
            im2_centre_of_mass.setZero();
            default_type im2_mass = 0.0;
            MR::Transform im2_transform (im2);
            for (auto i = Loop(0, 3)(im2); i; ++i) {
              Eigen::Vector3 voxel_pos ((default_type)im2.index(0), (default_type)im2.index(1), (default_type)im2.index(2));
              Eigen::Vector3 im2_scanner = im2_transform.voxel2scanner * voxel_pos;
              im2_mass += im2.value();
              im2_centre_of_mass += im2_scanner * im2.value();
            }
            im2_centre_of_mass /= im2_mass;

            Eigen::Vector3 centre = (im1_centre_of_mass + im2_centre_of_mass) / 2.0;
            Eigen::Vector3 translation = im1_centre_of_mass - im2_centre_of_mass;
            transform.set_centre (centre);
            transform.set_translation (translation);
            VAR(centre);
            VAR(translation);
        }
      }
    }
  }
}
#endif
