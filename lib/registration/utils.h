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

#include "algo/loop.h"
#include "transform.h"
#include "point.h"

namespace MR
{
  namespace Registration
  {

    template <class ImageType>
    void displacement2deformation (ImageType& input, ImageType& output) {
      Transform transform (input);
      auto kernel = [&] (InputImageType& input, OutputImageType& output) {
        Eigen::Vector3 voxel ((default_type)input.index(0), (default_type)input.index(1), (default_type)input.index(2));
        output.row(3) = (transform.voxel2scanner * voxel).template cast<typename ImageType::value_type> () + input.row(3);
      };
      ThreadedLoop (input, 0, 3).run (kernel, input, output);
    }

    template <class ImageType>
    void deformation2displacement (ImageType& input, ImageType& output) {
      Transform transform (input);
      auto kernel = [&] (InputImageType& input, OutputImageType& output) {
        Eigen::Vector3 voxel ((default_type)input.index(0), (default_type)input.index(1), (default_type)input.index(2));
        output.row(3) = input.row(3) - (transform.voxel2scanner * voxel).template cast<typename ImageType::value_type> ();
      };
      ThreadedLoop (input, 0, 3).run (kernel, input, output);
    }

  }
}
