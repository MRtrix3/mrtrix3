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


#ifndef __image_registration_symmetric_transform_initialiser_h__
#define __image_registration_symmetric_transform_initialiser_h__

#include "image/transform.h"
#include "image/loop.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "point.h"

namespace MR
{
  namespace Image
  {
    namespace RegistrationSymmetric
    {
      namespace Transform
      {
        namespace Init
        {
          enum InitType {mass, geometric, none};

          template <class MovingVoxelType,
                    class TargetVoxelType,
                    class TransformType>
            void initialise_using_image_centres (const MovingVoxelType& moving,
                                                 const TargetVoxelType& target,
                                                 TransformType& transform_moving2target)
            {
              CONSOLE ("initialising centre of rotation and translation using geometric centre (symmetric)");
              Point<double> moving_centre_voxel;
              moving_centre_voxel[0] = (static_cast<double>(moving.dim(0)) / 2.0) - 0.5;
              moving_centre_voxel[1] = (static_cast<double>(moving.dim(1)) / 2.0) - 0.5;
              moving_centre_voxel[2] = (static_cast<double>(moving.dim(2)) / 2.0) - 0.5;
              Image::Transform moving_transform (moving);
              Math::Vector<double> moving_centre_scanner (3);
              moving_transform.voxel2scanner (moving_centre_voxel, moving_centre_scanner);

              Point<double> target_centre_voxel;
              target_centre_voxel[0] = (static_cast<double>(target.dim(0)) / 2.0) - 0.5 + 1.0;
              target_centre_voxel[1] = (static_cast<double>(target.dim(1)) / 2.0) - 0.5 + 1.0;
              target_centre_voxel[2] = (static_cast<double>(target.dim(2)) / 2.0) - 0.5 + 1.0;
              Math::Vector<double> target_centre_scanner (3);
              moving_transform.voxel2scanner (target_centre_voxel, target_centre_scanner);

              Math::Vector<double> centre (3);
              centre =  target_centre_scanner;
              centre += moving_centre_scanner;
              centre *= 0.5;
              transform_moving2target.set_centre (centre); 
              moving_centre_scanner -= target_centre_scanner;
              transform_moving2target.set_translation (moving_centre_scanner); // translation from moving to target
              // transform_moving2target.set_centre (target_centre_scanner); 
              // transform_moving2target.set_translation (moving_centre_scanner);
            }

          template <class MovingVoxelType,
                    class TargetVoxelType,
                    class TransformType>
            void initialise_using_image_mass (const MovingVoxelType& moving,
                                              const TargetVoxelType& target,
                                              TransformType& transform_moving2target)
            {
              CONSOLE ("initialising centre of rotation and translation using centre of mass (symmetric)");
              Math::Vector<typename TransformType::ParameterType > target_centre_of_mass (3);
              target_centre_of_mass.zero();
              double target_mass = 0;
              MovingVoxelType target_voxel (target);
              Image::Transform target_transform (target_voxel);

              // only use the first volume of a 4D file. This is important for FOD images.
              Image::LoopInOrder target_loop (target_voxel, 0, 3);
              for (target_loop.start (target_voxel); target_loop.ok(); target_loop.next (target_voxel)) {
                Point<float> target_scanner = target_transform.voxel2scanner (target_voxel);
                target_mass += target_voxel.value();
                for (size_t dim = 0; dim < 3; dim++) {
                  target_centre_of_mass[dim] += target_scanner[dim] * target_voxel.value();
                }
              }

              Math::Vector<typename TransformType::ParameterType> moving_centre_of_mass (3);
              moving_centre_of_mass.zero();
              double moving_mass = 0;
              Image::Transform moving_transform (moving);
              MovingVoxelType moving_voxel (moving);
              Image::LoopInOrder moving_loop (moving_voxel);
              for (moving_loop.start (moving_voxel); moving_loop.ok(); moving_loop.next (moving_voxel)) {
                Point<float> moving_scanner = moving_transform.voxel2scanner (moving_voxel);
                moving_mass += moving_voxel.value();
                for (size_t dim = 0; dim < 3; dim++) {
                  moving_centre_of_mass[dim] += moving_scanner[dim] * moving_voxel.value();
                }
              }
              for (size_t dim = 0; dim < 3; dim++) {
                target_centre_of_mass[dim] /= target_mass;
                moving_centre_of_mass[dim] /= moving_mass;
              }

              Math::Vector<double> centre (3);
              centre =  target_centre_of_mass;
              centre += moving_centre_of_mass;
              centre *= 0.5;
              transform_moving2target.set_centre (centre); 
              moving_centre_of_mass -= target_centre_of_mass;
              transform_moving2target.set_translation (moving_centre_of_mass);
              // transform_moving2target.set_centre (target_centre_of_mass);
              // moving_centre_of_mass -= target_centre_of_mass;
              // transform_moving2target.set_translation (moving_centre_of_mass);
          }
        }
      }
    }
  }
}
#endif
