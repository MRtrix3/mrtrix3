/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __adapter_neighbourhood_h__
#define __adapter_neighbourhood_h__

#include "image.h"
#include "adapter/base.h"

namespace MR
{
  namespace Adapter {

    template <class ImageType>
      class NeighbourhoodCoord : 
        public Base<NeighbourhoodCoord<ImageType>,ImageType>
    { MEMALIGN (NeighbourhoodCoord<ImageType>)
      public:

        using base_type = Base<NeighbourhoodCoord<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;

        using base_type::name;
        using base_type::spacing;


        template <class VectorType>
          NeighbourhoodCoord (const ImageType& original, const VectorType& extent, const Iterator& iter) :
            base_type (original),
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

        ssize_t get_index (size_t axis) const { return parent().index(axis)-from_[axis]; }
        void move_index (size_t axis, ssize_t increment) { parent().index(axis) += increment; }

      protected:
        using base_type::parent;
        vector<ssize_t> from_, size_;
        Iterator iter_;
        transform_type transform_;
    };

  }
}

#endif


