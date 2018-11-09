/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __registration_warp_convert_h__
#define __registration_warp_convert_h__



#include "algo/loop.h"
#include "transform.h"

namespace MR
{
  namespace Registration
  {

    namespace Warp
    {

      template <class ImageType>
      void displacement2deformation (ImageType& input, ImageType& output) {
        MR::Transform transform (input);
        auto kernel = [&] (ImageType& input, ImageType& output) {
          Eigen::Vector3 voxel ((default_type)input.index(0), (default_type)input.index(1), (default_type)input.index(2));
          output.row(3) = (transform.voxel2scanner * voxel).template cast<typename ImageType::value_type> () + Eigen::Vector3 (input.row(3));
        };
        ThreadedLoop (input, 0, 3).run (kernel, input, output);
      }

      template <class ImageType>
      void deformation2displacement (ImageType& input, ImageType& output) {
        MR::Transform transform (input);
        auto kernel = [&] (ImageType& input, ImageType& output) {
          Eigen::Vector3 voxel ((default_type)input.index(0), (default_type)input.index(1), (default_type)input.index(2));
          output.row(3) = Eigen::Vector3(input.row(3)) - transform.voxel2scanner * voxel;
        };
        ThreadedLoop (input, 0, 3).run (kernel, input, output);
      }
    }
  }
}

#endif
