/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 22/02/12.

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

#ifndef __image_adapter_gradient1D_h__
#define __image_adapter_gradient1D_h__

#include "image/adapter/voxel.h"

namespace MR
{
  namespace Image
  {
    namespace Adapter
    {

      template <class VoxelType>
        class Gradient1D : public Voxel<VoxelType> {
        public:
          Gradient1D (const VoxelType& parent,
                      size_t axis = 0) :
            Voxel<VoxelType> (parent),
            axis_(axis) { }

          void set_axis(size_t axis) {
            axis_ = axis;
          }

          ssize_t value () {
            const ssize_t pos = (*this)[axis_];
            ssize_t val = 0.0;

            if (pos == 0) {
              val = parent_vox.value();
              (*this)[axis_] = pos + 1;
              val = parent_vox.value() - val;
            } else if (pos == dim(axis_) - 1) {
              val = parent_vox.value();
              (*this)[axis_] = pos - 1;
              val -= parent_vox.value();
            } else {
              (*this)[axis_] = pos + 1;
              val = 0.5 * parent_vox.value();
              (*this)[axis_] = pos - 1;
              val -= 0.5 * parent_vox.value();
            }
            (*this)[axis_] = pos;

            return val;
          }

          using Voxel<VoxelType>::name;
          using Voxel<VoxelType>::dim;
          using Voxel<VoxelType>::vox;
          using Voxel<VoxelType>::operator[];

        protected:
          using Voxel<VoxelType>::parent_vox;
          size_t axis_;
        };
    }
  }
}


#endif

