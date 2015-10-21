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

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {
      namespace Init
      {
        enum InitType {mass, geometric, none};

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
        }
      }
    }
  }
}
#endif
