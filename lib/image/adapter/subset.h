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
      private:
        class Axis
        {
          public:
            size_t dim, from;
        };

      public:
        typedef typename VoxelType::value_type value_type;

        Subset (const VoxelType& original, const size_t* from, const size_t* dimensions, const std::string& description = "") :
          Voxel<VoxelType> (original),
          axes (original.ndim()),
          descriptor (description.empty() ? name() + " [subset]" : description),
          transform_matrix (original.transform()) {
            for (size_t n = 0; n < ndim(); ++n) {
              assert (ssize_t (from[n] + dimensions[n]) <= original.dim (n));
              axes[n].from = from[n];
              axes[n].dim = dimensions[n];
            }

            for (size_t j = 0; j < 3; ++j)
              for (size_t i = 0; i < 3; ++i)
                transform_matrix (i,3) += axes[j].from * vox (j) * transform_matrix (i,j);
          }

        Subset (const VoxelType& original, size_t NDIM, const size_t* from, const size_t* dimensions, const std::string& description = "") :
          Voxel<VoxelType> (original),
          axes (NDIM),
          descriptor (description.empty() ? name() + " [subset]" : description),
          transform_matrix (original.transform()) {
            for (size_t n = 0; n < ndim(); ++n) {
              assert (ssize_t (from[n] + dimensions[n]) <= original.dim (n));
              axes[n].from = from[n];
              axes[n].dim = dimensions[n];
            }

            for (size_t j = 0; j < 3; ++j)
              for (size_t i = 0; i < 3; ++i)
                transform_matrix (i,3) += axes[j].from * vox (j) * transform_matrix (i,j);
          }

        const std::string& name () const {
          return descriptor;
        }
        size_t  ndim () const {
          return axes.size();
        }
        int dim (size_t axis) const {
          return axes[axis].dim;
        }
        const Math::Matrix<float>& transform () const {
          return transform_matrix;
        }

        void reset () {
          for (size_t n = 0; n < ndim(); ++n)
            set_pos (n, 0);
        }

        Value<Subset<VoxelType> > value () {
          return (Value<Subset<VoxelType> > (*this));
        }
        Position<Subset<VoxelType> > operator[] (size_t axis) {
          return (Position<Subset<VoxelType> > (*this, axis));
        }

        using Voxel<VoxelType>::name;
        using Voxel<VoxelType>::vox;
        using Voxel<VoxelType>::stride;

      protected:
        using Voxel<VoxelType>::vox_;
        std::vector<Axis> axes;
        std::string descriptor;
        Math::Matrix<float> transform_matrix;

        value_type get_value () const {
          return vox_.value();
        }
        void set_value (value_type val) {
          vox_.value() = val;
        }

        ssize_t get_pos (size_t axis) const {
          return (vox_[axis]-axes[axis].from);
        }
        void set_pos (size_t axis, ssize_t position) {
          vox_[axis] = position + axes[axis].from;
        }
        void move_pos (size_t axis, ssize_t increment) {
          vox_[axis] += increment;
        }

        friend class Value<Subset<VoxelType> >;
        friend class Position<Subset<VoxelType> >;
    };

    }
  }
}

#endif


