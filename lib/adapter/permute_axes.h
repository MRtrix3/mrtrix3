/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 02/11/09.

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

#ifndef __adapter_permute_axes_h__
#define __adapter_permute_axes_h__

#include "adapter/base.h"

namespace MR
{
  namespace Adapter
  {

    template <class ImageType> 
      class PermuteAxes : public Base<ImageType>
    {
      public:
        using Base<ImageType>::size;
        using Base<ImageType>::parent_vox;
        typedef typename ImageType::value_type value_type;

        PermuteAxes (const ImageType& original, const std::vector<int>& axes) :
          Base<ImageType> (original), 
          axes_ (axes) {
            for (int i = 0; i < static_cast<int> (parent_vox.ndim()); ++i) {
              for (size_t a = 0; a < axes_.size(); ++a)
                if (axes_[a] == i)
                  goto next_axis;
              if (parent_vox.size (i) != 1)
                throw Exception ("ommitted axis \"" + str (i) + "\" has dimension greater than 1");
next_axis:
              continue;
            }
          }

        size_t ndim () const {
          return axes_.size();
        }
        ssize_t size (size_t axis) const {
          return axes_[axis] < 0 ? 1 : parent_vox.size (axes_[axis]);
        }
        default_type voxsize (size_t axis) const {
          return axes_[axis] < 0 ? std::numeric_limits<default_type>::quiet_NaN() : parent_vox.voxsize (axes_[axis]);
        }
        ssize_t stride (size_t axis) const {
          return axes_[axis] < 0 ? 0 : parent_vox.stride (axes_[axis]);
        }

        void reset () { parent_vox.reset(); }

        Helper::VoxelIndex<PermuteAxes<ImageType> > index (size_t axis) {
          return Helper::VoxelIndex<PermuteAxes<ImageType> > (*this, axis);
        }

      private:
        const std::vector<int> axes_;

        ssize_t get_voxel_position (size_t axis) {
          return axes_[axis] < 0 ? 0 : parent_vox.index (axes_[axis]);
        }
        void set_voxel_position (size_t axis, ssize_t position) {
          parent_vox.index (axes_[axis]) = position;
        }
        void move_voxel_position (size_t axis, ssize_t increment) {
          parent_vox.index (axes_[axis]) += increment;
        }

        friend class Helper::VoxelIndex<PermuteAxes<ImageType>>;
        friend class Helper::VoxelValue<PermuteAxes<ImageType>>;
    };

  }
}

#endif


