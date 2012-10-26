/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 22/10/09.

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

#ifndef __image_adapter_replicate_h__
#define __image_adapter_replicate_h__

#include "math/matrix.h"
#include "image/info.h"
#include "image/position.h"
#include "image/voxel.h"
#include "image/adapter/voxel.h"

namespace MR
{
  namespace Image
  {
    namespace Adapter {

    template <class VoxelType>
      class Replicate : public Voxel<VoxelType>
    {
      public:
        typedef typename VoxelType::value_type value_type;

        using Voxel<VoxelType>::name;
        using Voxel<VoxelType>::ndim;
        using Voxel<VoxelType>::vox;

        template <class InfoType>
          Replicate (VoxelType& original, const InfoType& replication_template) :
            Voxel<VoxelType> (original),
            info_ (replication_template),
            pos_ (ndim(), 0)
          {
            for (size_t n = 0; n < std::min (parent_vox.ndim(), info_.ndim()); ++n) {
              if (parent_vox.dim(n) > 1 && parent_vox.dim(n) != info_.dim(n))
                throw Exception ("cannot replicate over non-singleton dimensions");
            }
          }

        const Image::Info& info() const {
          return info_;
        }

        size_t ndim () const {
          return info_.ndim();
        }
        ssize_t dim (size_t axis) const {
          return info_.dim (axis);
        }
        float vox (size_t axis) const {
          return info_.vox(axis);
        }
        ssize_t stride (size_t axis) const {
          return axis < parent_vox.ndim() ? parent_vox.stride (axis) : 0;
        }

        Position<Replicate<VoxelType> > operator[] (size_t axis) {
          return (Position<Replicate<VoxelType> > (*this, axis));
        }

      protected:
        using Voxel<VoxelType>::parent_vox;
        Image::Info info_;
        std::vector<ssize_t> pos_;

        ssize_t get_pos (size_t axis) const {
          return pos_[axis];
        }
        void set_pos (size_t axis, ssize_t position) {
          pos_[axis] = position;
          if (axis < parent_vox.ndim())
            if (parent_vox.dim(axis) > 1)
              parent_vox[axis] = position;
        }
        void move_pos (size_t axis, ssize_t increment) {
          pos_[axis] += increment;
          if (axis < parent_vox.ndim())
            if (parent_vox.dim(axis) > 1)
              parent_vox[axis] += increment;
        }

        friend class Position<Replicate<VoxelType> >;
    };

    }
  }
}

#endif



