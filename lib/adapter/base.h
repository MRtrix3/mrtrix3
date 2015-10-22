/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 23/05/09.

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

#ifndef __algo_adapter_base_h__
#define __algo_adapter_base_h__

#include "image_helpers.h"

namespace MR
{
  class Header;

  namespace Adapter
  {

    template <template <class AdapterType> class AdapterType, class ImageType, typename... Args>
      inline AdapterType<ImageType> make (const ImageType& parent, Args&&... args) {
        return AdapterType<ImageType> (parent, std::forward<Args> (args)...);
      }

    template <class ImageType> 
      class Base {
        public:
          Base (const ImageType& parent) :
              parent_ (parent) { }

          typedef typename ImageType::value_type value_type;

          template <class U> 
            const Base& operator= (const U& V) { return parent_ = V; }

          FORCE_INLINE const Header& header () const { return parent_.header(); }
          FORCE_INLINE ImageType& parent () { return parent_; }
          FORCE_INLINE bool valid () const { return parent_.valid(); }
          FORCE_INLINE bool operator! () const { return !valid(); }
          FORCE_INLINE const ImageType& parent () const { return parent_; }
          FORCE_INLINE const std::string& name () const { return parent_.name(); }
          FORCE_INLINE size_t ndim () const { return parent_.ndim(); }
          FORCE_INLINE ssize_t size (size_t axis) const { return parent_.size (axis); }
          FORCE_INLINE default_type spacing (size_t axis) const { return parent_.spacing (axis); }
          FORCE_INLINE ssize_t stride (size_t axis) const { return parent_.stride (axis); }
          FORCE_INLINE const transform_type& transform () const { return parent_.transform(); }

          FORCE_INLINE ssize_t index (size_t axis) const { return parent_.index (axis); }
          FORCE_INLINE auto index (size_t axis) -> decltype(Helper::index(*this, axis)) { return { *this, axis }; }
          FORCE_INLINE void move_index (size_t axis, ssize_t increment) { parent_.index (axis) += increment; }

          FORCE_INLINE value_type value () const { return parent_.value(); } 
          FORCE_INLINE auto value () -> decltype(Helper::value(*this)) { return { *this }; }
          FORCE_INLINE void set_value (value_type val) { parent_.value() = val; } 

          FORCE_INLINE void reset () { parent_.reset(); }

          friend std::ostream& operator<< (std::ostream& stream, const Base& V) {
            stream << "image adapter \"" << V.name() << "\", datatype " << MR::DataType::from<value_type>().specifier() << ", position [ ";
            for (size_t n = 0; n < V.ndim(); ++n) 
              stream << V[n] << " ";
            stream << "], value = " << V.value();
            return stream;
          }

        protected:
          ImageType parent_;
      };


  }
}

#endif





