/*
    Copyright 2013 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 2013

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

#ifndef __image_registration_transform_reorient_h__
#define __image_registration_transform_reorient_h__

#include "image/threaded_loop.h"

namespace MR
{
  namespace Image
  {
    namespace Registration
    {
      namespace Transform
      {

        template <class WarpVoxelType, class FODVoxelType>
        class ReorientAdapter {
          public:

          private:
            WarpVoxelType warp;
            FODVoxelType fod;
        };

        void reorient_FOD (const Math::Matrix<float>& transform, Math::Vector<float>& FOD) {

        }

        template <class FODVoxelType>
        void reorient (const Math::Matrix<float>& transform, FODVoxelType& target)
        {
          Image::ThreadedLoop
        }

        template <class WarpVoxelType, class FODVoxelType>
        void reorient (const WarpVoxelType& warp, FODVoxelType& target)
        {
          // for each warp component compute gradient


        }


      }
    }
  }
}

#endif
