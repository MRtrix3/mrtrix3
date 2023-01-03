/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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


#ifndef __adapter_regrid_h__
#define __adapter_regrid_h__

#include "image.h"
#include "adapter/base.h"

namespace MR
{
  namespace Adapter {

    template <class ImageType>
      class Regrid :
        public Base<Regrid<ImageType>,ImageType>
    { MEMALIGN(Regrid<ImageType>)
      public:

        using base_type = Base<Regrid<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;

        using base_type::name;
        using base_type::spacing;

        template <class VectorType>
          Regrid (const ImageType& original, const VectorType& from, const VectorType& size, const value_type fill = 0) :
            base_type (original),
            from_ (container_cast<decltype(from_)>(from)),
            size_ (container_cast<decltype(size_)>(size)),
            index_invalid_lower_upper ([&]{ vector<vector<ssize_t>> v; for (size_t d = 0; d < from_.size(); ++d) {
              v.push_back (vector<ssize_t> {from_[d] < 0 ? -(ssize_t) from_[d] - 1 : -1, original.size(d) - from_[d]}); } return v; }()),
            index_requires_bound_check ([&]{ vector<bool> v; for (size_t d = 0; d < from_.size(); ++d) {
              v.push_back (from_[d] < 0 || size_[d] > original.size(d) - from_[d]); } return v; }()),
            fill_ (fill),
            transform_ (original.transform()),
            index_ (ndim(), 0) {
              assert(from_.size() == size_.size());
              assert(from_.size() == ndim());

              for (size_t n = 0; n < ndim(); ++n)
                if (size_[n] < 0)
                  throw Exception("FIXME: negative size in Regrid adapter");

              // adjust location of origin
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

        ssize_t get_index (size_t axis) const {
          return index_requires_bound_check[axis] ? index_[axis] : parent().index(axis) - from_[axis];
        }

        void move_index (size_t axis, ssize_t increment) {
          if (index_requires_bound_check[axis]) {
            index_[axis] += increment;
            if (increment > 0) {
              if (index_[axis] < index_invalid_lower_upper[axis][1])
                parent().index(axis) = index_[axis] + from_[axis];
            } else
              if (index_[axis] > index_invalid_lower_upper[axis][0])
                parent().index(axis) = index_[axis] + from_[axis];
          } else
            parent().index(axis) += increment;
        }

        value_type value () {
          for (size_t axis = 0; axis < index_.size(); ++axis)
            if (index_requires_bound_check[axis] &&
              (index_[axis] >= index_invalid_lower_upper[axis][1] || index_[axis] <= index_invalid_lower_upper[axis][0]))
              return fill_;
          return parent().value();
        }

      protected:
        using base_type::parent;
        const vector<ssize_t> from_, size_;
        const vector<vector<ssize_t>> index_invalid_lower_upper;
        const vector<bool> index_requires_bound_check;
        const value_type fill_;
        transform_type transform_;
        vector<ssize_t> index_;
    };

  }
}

#endif


