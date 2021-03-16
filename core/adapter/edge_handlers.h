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

#ifndef __adapter_edgehandlers_h__
#define __adapter_edgehandlers_h__

#include "adapter/base.h"

namespace MR
{
  namespace Adapter
  {



    template <class ImageType>
    class EdgeCrop : public Base<EdgeCrop<ImageType>,ImageType>
    { MEMALIGN(EdgeCrop<ImageType>)
      public:

        using base_type = Base<EdgeCrop<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;

        EdgeCrop (ImageType& original, const value_type default_value = std::numeric_limits<value_type>::quiet_NaN()) :
            base_type (original),
            _default_value (default_value) { }

        value_type get_value () const {
          if (is_out_of_bounds (parent()))
            return _default_value;
          return parent().value();
        }

      protected:
        using base_type::parent;
        const value_type _default_value;
    };



    template <class ImageType>
    class EdgeExtend : public Base<EdgeExtend<ImageType>,ImageType>
    { MEMALIGN(EdgeExtend<ImageType>)
      public:

        using base_type = Base<EdgeExtend<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;

        EdgeExtend (ImageType& original) :
            base_type (original),
            pos_ (original.ndim(), 0) { }

        ssize_t get_index (size_t axis) const {
          return pos_[axis];
        }
        void move_index (size_t axis, ssize_t increment) {
          pos_[axis] += increment;
          if (axis < 3)
            parent().index(axis) = std::min (parent().size(axis) - 1, std::max (ssize_t(0), pos_[axis]));
        }

      protected:
        using base_type::parent;
        vector<ssize_t> pos_;
    };



    template <class ImageType>
    class EdgeMirror : public Base<EdgeMirror<ImageType>,ImageType>
    { MEMALIGN(EdgeMirror<ImageType>)
      public:

        using base_type = Base<EdgeExtend<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;

        EdgeMirror (ImageType& original) :
            base_type (original),
            pos_ (original.ndim(), 0) { }

        ssize_t get_index (size_t axis) const {
          return pos_[axis];
        }
        void move_index (size_t axis, ssize_t increment) {
          pos_[axis] += increment;
          if (axis < 3) {
            if (pos_[axis] < 0)
              parent().index(axis) = abs (pos_[axis]);
            else if (pos_[axis] >= parent().size(axis))
              parent().index(axis) = 2 * parent().size(axis) - pos_[axis] - 2;
          }
        }

      protected:
        using base_type::parent;
        vector<ssize_t> pos_;
    };







  }
}

#endif



