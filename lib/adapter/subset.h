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

#ifndef __adapter_subset_h__
#define __adapter_subset_h__

#include "math/matrix.h"
#include "image.h"
#include "adapter/base.h"

namespace MR
{
  namespace Adapter {

    template <class ImageType>
      class Subset : public Base<ImageType>
    {
      public:
        typedef typename ImageType::value_type value_type;

        using Base<ImageType>::name;
        using Base<ImageType>::voxsize;

        template <class VectorType>
          Subset (const ImageType& original, const VectorType& from, const VectorType& dimensions) :
            Base<ImageType> (original),
            from_ (container_cast<decltype(from_)>(from)),
            size_ (container_cast<decltype(size_)>(dimensions)),
            transform_ (original.transform())
      {
        for (size_t n = 0; n < ndim(); ++n) 
          if (from_[n] + size_[n] > original.size(n))
            throw Exception ("FIXME: dimensions requested for Subset adapter are out of bounds!");

        for (size_t j = 0; j < 3; ++j)
          for (size_t i = 0; i < 3; ++i)
            transform_(i,3) += from[j] * voxsize(j) * transform_(i,j);
      }

        void reset () {
          for (size_t n = 0; n < ndim(); ++n)
            set_pos (n, 0);
        }

        size_t ndim () const { return size_.size(); }
        ssize_t size (size_t axis) const { return size_ [axis]; }
        const Math::Matrix<float>& transform() const { return transform_; }

        auto index (size_t axis) -> decltype(Helper::voxel_index(*this, axis)) { return { *this, axis }; } 
        ssize_t index (size_t axis) const { return get_voxel_position (axis); }


      protected:
        using Base<ImageType>::parent;
        const std::vector<ssize_t> from_, size_;
        Math::Matrix<default_type> transform_;

        ssize_t get_voxel_position (size_t axis) const {
          return parent().index(axis)-from_[axis];
        }
        void set_voxel_position (size_t axis, ssize_t position) {
          parent().index(axis) = position + from_[axis];
        }
        void move_voxel_position (size_t axis, ssize_t increment) {
          parent().index(axis) += increment;
        }

        friend class Helper::VoxelIndex<Subset<ImageType>>;
    };

  }
}

#endif


