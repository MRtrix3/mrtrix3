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

#include "image/transform.h"
#include "image/loop.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "point.h"

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

      template <class MovingVoxelType, class TargetVoxelType, class TransformType>
        void initialise_using_image_centres (const MovingVoxelType& moving, const TargetVoxelType& target, TransformType& transform) {

          Point<double> moving_centre_voxel;
          moving_centre_voxel[0] = (static_cast<double>(moving.dim(0)) / 2.0) - 0.5;
          moving_centre_voxel[1] = (static_cast<double>(moving.dim(1)) / 2.0) - 0.5;
          moving_centre_voxel[2] = (static_cast<double>(moving.dim(2)) / 2.0) - 0.5;
          Image::Transform moving_transform (moving);
          Math::Vector<double> moving_centre_scanner (3);
          moving_transform.voxel2scanner (moving_centre_voxel, moving_centre_scanner);

          Point<double> target_centre_voxel;
          target_centre_voxel[0] = (static_cast<double>(target.dim(0)) / 2.0) - 0.5;
          target_centre_voxel[1] = (static_cast<double>(target.dim(1)) / 2.0) - 0.5;
          target_centre_voxel[2] = (static_cast<double>(target.dim(2)) / 2.0) - 0.5;
          Image::Transform target_transform (target);
          Math::Vector<double> target_centre_scanner (3);
          moving_transform.voxel2scanner (target_centre_voxel, target_centre_scanner);

          transform.set_centre (target_centre_scanner);
          moving_centre_scanner -= target_centre_scanner;
          transform.set_translation (moving_centre_scanner);
        }

      template <class MovingVoxelType, class TargetVoxelType, class TransformType>
        void initialise_using_image_gravity (const MovingVoxelType& moving, const TargetVoxelType& target, TransformType& transform) {

//          Image::LoopInOrder target_loop (target);
//          Matrix<float> vox2scanner = ;
//          Point scanner_coord;
//
//          for (target_loop.start(target); target_loop.ok(); target_loop.next(target)) {
//            Image::Transform::voxel2scanner()
//          }


      }
    }
  }
}
#endif
