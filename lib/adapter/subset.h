/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __adapter_subset_h__
#define __adapter_subset_h__

#include "image.h"
#include "adapter/base.h"

namespace MR
{
  namespace Adapter {

    template <class ImageType>
      class Subset : public Base<ImageType>
    {
      public:
        using value_type = typename ImageType::value_type;

        using Base<ImageType>::name;
        using Base<ImageType>::spacing;

        template <class VectorType>
          Subset (const ImageType& original, const VectorType& from, const VectorType& size) :
            Base<ImageType> (original),
            from_ (container_cast<decltype(from_)>(from)),
            size_ (container_cast<decltype(size_)>(size)),
            transform_ (original.transform()) {

              for (size_t n = 0; n < ndim(); ++n) 
                if (from_[n] + size_[n] > original.size(n))
                  throw Exception ("FIXME: dimensions requested for Subset adapter are out of bounds!");

              for (size_t j = 0; j < 3; ++j)
                for (size_t i = 0; i < 3; ++i)
                  transform_(i,3) += from[j] * spacing(j) * transform_(i,j);
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
        const std::vector<ssize_t> from_, size_;
        transform_type transform_;
    };

  }
}

#endif


