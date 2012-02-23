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

#ifndef __image_adapter_subset_h__
#define __image_adapter_subset_h__

#include "math/matrix.h"
#include "image/value.h"
#include "image/voxel.h"
#include "image/position.h"
#include "image/adapter/voxel.h"

namespace MR
{
  namespace Image
  {
    namespace Adapter {

    template <class VoxelType>
      class Subset : public Voxel<VoxelType>
    {
      public:
        typedef typename VoxelType::value_type value_type;

        using Voxel<VoxelType>::name;
        using Voxel<VoxelType>::ndim;
        using Voxel<VoxelType>::vox;

        template <class VectorType>
          Subset (const VoxelType& original, const VectorType& from, const VectorType& dimensions) :
            Voxel<VoxelType> (original),
            from_ (ndim()) {
              for (size_t n = 0; n < ndim(); ++n) {
                assert (ssize_t (from[n] + dimensions[n]) <= original.dim(n));
                from_[n] = from[n];
                dim_[n] = dimensions[n];
              }

              for (size_t j = 0; j < 3; ++j)
                for (size_t i = 0; i < 3; ++i)
                  transform_(i,3) += from[j] * vox(j) * transform_(i,j);
            }

        void reset () {
          for (size_t n = 0; n < ndim(); ++n)
            set_pos (n, 0);
        }

        ssize_t dim (size_t axis) const {
          return dim_[axis];
        }
        const Math::Matrix<float>& transform() const { 
          return transform_;
        }

        Value<Subset<VoxelType> > value () {
          return (Value<Subset<VoxelType> > (*this));
        }
        Position<Subset<VoxelType> > operator[] (size_t axis) {
          return (Position<Subset<VoxelType> > (*this, axis));
        }

      protected:
        using Voxel<VoxelType>::parent_vox;
        std::vector<ssize_t> from_;
        std::vector<ssize_t> dim_;
        Math::Matrix<float> transform_;

        value_type get_value () const {
          return parent_vox.value();
        }
        void set_value (value_type val) {
          parent_vox.value() = val;
        }

        ssize_t get_pos (size_t axis) const {
          return parent_vox[axis]-from_[axis];
        }
        void set_pos (size_t axis, ssize_t position) {
          parent_vox[axis] = position + from_[axis];
        }
        void move_pos (size_t axis, ssize_t increment) {
          parent_vox[axis] += increment;
        }

        friend class Value<Subset<VoxelType> >;
        friend class Position<Subset<VoxelType> >;
    };

    }
  }
}

#endif


