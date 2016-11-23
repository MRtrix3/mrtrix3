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

#ifndef __adapter_replicate_h__
#define __adapter_replicate_h__

#include "header.h"
#include "image_helpers.h"
#include "adapter/base.h"

namespace MR
{
    namespace Adapter
    {

    template <class ImageType>
      class Replicate : public Base<ImageType> { MEMALIGN(Replicate<ImageType>)
      public:
        typedef typename ImageType::value_type value_type;

        using Base<ImageType>::name;
        using Base<ImageType>::ndim;
        using Base<ImageType>::spacing;

          Replicate (ImageType& original, const Header& replication_template) :
            Base<ImageType> (original),
            header_ (replication_template),
            pos_ (ndim(), 0)
          {
            for (size_t n = 0; n < std::min<size_t> (parent().ndim(), header_.ndim()); ++n) {
              if (parent().size(n) > 1 && parent().size(n) != header_.size(n))
                throw Exception ("cannot replicate over non-singleton dimensions");
            }
          }

        size_t ndim () const {
          return header_.ndim();
        }
        ssize_t size (size_t axis) const {
          return header_.size (axis);
        }
        float spacing (size_t axis) const {
          return header_.spacing(axis);
        }
        ssize_t stride (size_t axis) const {
          return axis < parent().ndim() ? parent().stride (axis) : 0;
        }

        Helper::Index<Replicate<ImageType> > operator[] (size_t axis) {
          return (Helper::Index<Replicate<ImageType> > (*this, axis));
        }

      protected:
        using Base<ImageType>::parent;
        Header header_;
        std::vector<ssize_t> pos_;

        ssize_t get_pos (size_t axis) const {
          return pos_[axis];
        }
        void set_pos (size_t axis, ssize_t position) {
          pos_[axis] = position;
          if (axis < parent().ndim())
            if (parent().dim(axis) > 1)
              parent()[axis] = position;
        }
        void move_pos (size_t axis, ssize_t increment) {
          pos_[axis] += increment;
          if (axis < parent().ndim())
            if (parent().dim(axis) > 1)
              parent()[axis] += increment;
        }

        friend class Helper::Index<Replicate<ImageType> >;
    };

    }
}

#endif



