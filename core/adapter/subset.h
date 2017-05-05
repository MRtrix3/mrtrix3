/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __adapter_subset_h__
#define __adapter_subset_h__

#include "image.h"
#include "adapter/base.h"

namespace MR
{
  namespace Adapter {

    template <class ImageType>
      class Subset : 
        public Base<Subset<ImageType>,ImageType>
    { MEMALIGN(Subset<ImageType>) 
      public:

        using base_type = Base<Subset<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;

        using base_type::name;
        using base_type::spacing;

        template <class VectorType>
          Subset (const ImageType& original, const VectorType& from, const VectorType& size) :
            base_type (original),
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

        ssize_t get_index (size_t axis) const { return parent().index(axis)-from_[axis]; }
        void move_index (size_t axis, ssize_t increment) { parent().index(axis) += increment; }

      protected:
        using base_type::parent;
        const vector<ssize_t> from_, size_;
        transform_type transform_;
    };

  }
}

#endif


