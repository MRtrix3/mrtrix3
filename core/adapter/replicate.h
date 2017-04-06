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
      class Replicate : 
        public Base<Replicate<ImageType>,ImageType>
    { MEMALIGN(Replicate<ImageType>)
      public:

        using base_type = Base<Replicate<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;

        using base_type::name;
        using base_type::ndim;
        using base_type::spacing;

          Replicate (ImageType& original, const Header& replication_template) :
            base_type (original),
            header_ (replication_template),
            pos_ (std::max<size_t> (parent().ndim(), header_.ndim()), 0)
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

        ssize_t get_index (size_t axis) const {
          return pos_[axis];
        }
        void move_index (size_t axis, ssize_t increment) {
          pos_[axis] += increment;
          if (axis < parent().ndim())
            if (parent().size(axis) > 1)
              parent().index(axis) += increment;
        }

      protected:
        using base_type::parent;
        Header header_;
        std::vector<ssize_t> pos_;

    };

    }
}

#endif



