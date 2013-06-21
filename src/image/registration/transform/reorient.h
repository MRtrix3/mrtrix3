/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

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


namespace MR
{
  namespace Image
  {
    namespace Registration
    {
      namespace Transform
      {

        template <class FODVoxelType>
        void reorient (const Math::Matrix<float>& transform, FODVoxelType& target)
        {
        }

        template <class WarpVoxelType, class FODVoxelType>
        void reorient (const WarpVoxelType& transform, FODVoxelType& target)
        {
        }


      }
    }
  }
}

#endif
