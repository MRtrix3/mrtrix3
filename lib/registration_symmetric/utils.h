/*
 Copyright 2012 Brain Research Institute, Melbourne, Australia

 Written by David Raffelt, 20/11/2012

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

#include "image/loop.h"
#include "image/transform.h"
#include "point.h"

namespace MR
{
  namespace Image
  {
    namespace RegistrationSymmetric
    {

      template <class InputVoxelType, class OutputVoxelType>
      void displacement2deformation (InputVoxelType& input, OutputVoxelType& output) {
        Image::LoopInOrder loop (input, 0, 3);
        Image::Transform transform (input);
        for (loop.start (input); loop.ok(); loop.next (input)) {
          voxel_assign (output, input, 0, 3);
          Point<float> point = transform.voxel2scanner (input);
          for (size_t dim = 0; dim < 3; ++dim) {
            input[3] = dim;
            output[3] = dim;
            output.value() = point[dim] + input.value();
          }
        }
      }

      template <class InputVoxelType, class OutputVoxelType>
      void deformation2displacement (InputVoxelType& input, OutputVoxelType& output) {
        Image::LoopInOrder loop (input, 0, 3);
        Image::Transform transform (input);
        for (loop.start (input); loop.ok(); loop.next (input)) {
          voxel_assign (output, input, 0, 3);
          Point<float> point = transform.voxel2scanner (input);
          for (size_t dim = 0; dim < 3; ++dim) {
            input[3] = dim;
            output[3] = dim;
            output.value() = input.value() - point[dim];
          }
        }
      }

    }
  }
}
