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

#ifndef __image_adapter_allow_empty_h__
#define __image_adapter_allow_empty_h__

#include "image.h"
#include "adapter/base.h"

namespace MR
{
  namespace Adapter {

    template <class ImageType>
      class AllowEmpty : public Base<ImageType>
    {
      public:
        typedef typename ImageType::value_type value_type;

        using Base<ImageType>::valid;

        AllowEmpty (const ImageType& original, value_type value_if_empty = value_type (0.0)) :
          Base<ImageType> (original),
          value_if_empty (value_if_empty) { }

        FORCE_INLINE void reset () {
          if (valid())
            for (size_t n = 0; n < ndim(); ++n)
              set_pos (n, 0);
        }

        FORCE_INLINE const Header& header () const { 
          if (valid()) 
            return parent().header(); 
          throw Exception ("FIXME: attempt to access header information from invalid image!"); }

        FORCE_INLINE const std::string& name() const { return valid() ? parent().name() : std::string("<empty image>"); }
        FORCE_INLINE const transform_type& transform() const { return valid() ? parent().transform() : transform_type(); }

        FORCE_INLINE size_t  ndim () const { return valid() ? parent().ndim() : 0; }
        FORCE_INLINE ssize_t size (size_t axis) const { return valid() ? parent().size(axis) : 0; }
        FORCE_INLINE default_type spacing (size_t axis) const { return valid() ? parent().spacing (axis) : NaN; }
        FORCE_INLINE ssize_t stride (size_t axis) const { return valid() ? parent().stride (axis) : 0; }

        FORCE_INLINE ssize_t index (size_t axis) const { return valid() ? parent().index (axis) : 0; }
        FORCE_INLINE auto index (size_t axis) -> decltype(Helper::index(*this, axis)) { return { *this, axis }; } 
        FORCE_INLINE void move_index (size_t axis, ssize_t increment) { if (valid()) parent().index(axis) += increment; }

        FORCE_INLINE value_type value () const { return valid() ? parent().value() : value_if_empty; } 
        FORCE_INLINE auto value () -> decltype(Helper::value(*this)) { return { *this }; }
        FORCE_INLINE void set_value (value_type val) { if (valid()) parent().value() = val; } 


      protected:
        using Base<ImageType>::parent;
        const value_type value_if_empty;
    };

    template <class ImageType>
      inline AllowEmpty<ImageType> allow_empty (const ImageType& parent, typename ImageType::value_type value_if_empty = typename ImageType::value_type (0.0)) {
        return AllowEmpty<ImageType> (parent, value_if_empty);
      }

  }
}

#endif


