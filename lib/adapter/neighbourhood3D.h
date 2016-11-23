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

#ifndef __adapter_neighbourhood_h__
#define __adapter_neighbourhood_h__

#include "image.h"
#include "adapter/base.h"

namespace MR
{
  namespace Adapter {

    template <class ImageType>
      class NeighbourhoodCoord : public Base<ImageType>
    { MEMALIGN (NeighbourhoodCoord<ImageType>)
      public:
        typedef typename ImageType::value_type value_type;

        using Base<ImageType>::name;
        using Base<ImageType>::spacing;


        template <class VectorType>
          NeighbourhoodCoord (const ImageType& original, const VectorType& extent, const Iterator& iter) :
            Base<ImageType> (original),
            iter_ (iter),
            transform_ (original.transform()) {

              assert (extent.size() == ndim());
              from_.resize(original.ndim());
              size_.resize(original.ndim());
              for (size_t i = 0; i < ndim(); ++i){
                from_[i] = (iter_.index(i)-extent[i] < 0) ? 0 : iter_.index(i)-extent[i];
                size_[i] = (from_[i] + extent[i] >= original.size(i)) ? original.size(i) - from_[i] -1 : extent[i];
                assert (from_[n] + size_[n] < original.size(n));
              }

              // for (size_t n = 0; n < ndim(); ++n) 
              //   if (from_[n] + size_[n] > original.size(n))
              //     throw Exception ("FIXME: dimensions requested for NeighbourhoodCoord adapter are out of bounds!");

              for (size_t j = 0; j < 3; ++j)
                for (size_t i = 0; i < 3; ++i)
                  transform_(i,3) += from_[j] * spacing(j) * transform_(i,j);
            }

        void reset () {
          for (size_t n = 0; n < ndim(); ++n)
            set_pos (n, 0);
        }

        size_t ndim () const { return size_.size(); }
        ssize_t size (size_t axis) const { return size_ [axis]; }
        const transform_type& transform() const { return transform_; }

        ssize_t index (size_t axis) const { return parent().index(axis)-from_[axis]; }
        auto index (size_t axis) -> decltype(Helper::index(*this, axis)) { return { *this, axis }; } 
        void move_index (size_t axis, ssize_t increment) { parent().index(axis) += increment; }

      protected:
        using Base<ImageType>::parent;
        std::vector<ssize_t> from_, size_;
        Iterator iter_;
        transform_type transform_;
    };

  }
}

#endif


