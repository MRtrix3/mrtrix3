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

        template <class MovingImageType,
                  class TargetImageType,
                  class TransformType>
          void initialise_using_image_centres (const MovingImageType& moving,
                                               const TargetImageType& target,
                                               TransformType& transform)
          {
            CONSOLE ("initialising centre of rotation and translation using geometric centre");
            Eigen::Vector3 moving_centre_voxel;
            moving_centre_voxel[0] = (static_cast<default_type>(moving.size(0)) / 2.0) - 0.5;
            moving_centre_voxel[1] = (static_cast<default_type>(moving.size(1)) / 2.0) - 0.5;
            moving_centre_voxel[2] = (static_cast<default_type>(moving.size(2)) / 2.0) - 0.5;
            MR::Transform moving_transform (moving);
            Eigen::Vector3 moving_centre_scanner  = moving_transform.voxel2scanner * moving_centre_voxel;

            Eigen::Vector3 target_centre_voxel;
            target_centre_voxel[0] = (static_cast<default_type>(target.size(0)) / 2.0) - 0.5 + 1.0;
            target_centre_voxel[1] = (static_cast<default_type>(target.size(1)) / 2.0) - 0.5 + 1.0;
            target_centre_voxel[2] = (static_cast<default_type>(target.size(2)) / 2.0) - 0.5 + 1.0;
            Eigen::Vector3 target_centre_scanner = moving_transform.voxel2scanner * target_centre_voxel;

            transform.set_centre (target_centre_scanner);
            moving_centre_scanner -= target_centre_scanner;
            transform.set_translation (moving_centre_scanner);
          }

        template <class MovingImageType,
                  class TargetImageType,
                  class TransformType>
          void initialise_using_image_mass (MovingImageType& moving,
                                            TargetImageType& target,
                                            TransformType& transform)
          {
            CONSOLE ("initialising centre of rotation and translation using centre of mass");
            Eigen::Matrix<typename TransformType::ParameterType, 3, 1> target_centre_of_mass (3);
            target_centre_of_mass.setZero();
            default_type target_mass = 0;
            MR::Transform target_transform (target);

            // only use the first volume of a 4D file. This is important for FOD images.
            for (auto i = Loop(0, 3)(target); i; ++i) {
              Eigen::Vector3 voxel_pos ((default_type)target.index(0), (default_type)target.index(1), (default_type)target.index(2));
              Eigen::Vector3 target_scanner = target_transform.voxel2scanner * voxel_pos;
              target_mass += target.value();
              for (size_t dim = 0; dim < 3; dim++) {
                target_centre_of_mass[dim] += target_scanner[dim] * target.value();
              }
            }

            Eigen::Matrix<typename TransformType::ParameterType, 3, 1>  moving_centre_of_mass (3);
            moving_centre_of_mass.setZero();
            default_type moving_mass = 0.0;
            MR::Transform moving_transform (moving);
            for (auto i = Loop (0, 3)(moving); i; ++i) {
              Eigen::Vector3 voxel_pos ((default_type)moving.index(0), (default_type)moving.index(1), (default_type)moving.index(2));
              Eigen::Vector3 moving_scanner = moving_transform.voxel2scanner * voxel_pos;
              moving_mass += moving.value();
              for (size_t dim = 0; dim < 3; dim++) {
                moving_centre_of_mass[dim] += moving_scanner[dim] * moving.value();
              }
            }
            for (size_t dim = 0; dim < 3; dim++) {
              target_centre_of_mass[dim] /= target_mass;
              moving_centre_of_mass[dim] /= moving_mass;
            }

            transform.set_centre (target_centre_of_mass);
            moving_centre_of_mass -= target_centre_of_mass;
            transform.set_translation (moving_centre_of_mass);
        }
      }
    }
  }
}
#endif
