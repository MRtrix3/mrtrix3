/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __algo_adapter_base_h__
#define __algo_adapter_base_h__

#include "image_helpers.h"

namespace MR
{
  class Header;

  namespace Adapter
  {

    template <template <class ImageType> class AdapterType, class ImageType, typename... Args>
      inline AdapterType<ImageType> make (const ImageType& parent, Args&&... args) {
        return AdapterType<ImageType> (parent, std::forward<Args> (args)...);
      }

    template <class AdapterType, class ImageType> 
      class Base : 
        public ImageBase<AdapterType, typename ImageType::value_type> 
    { MEMALIGN (Base<AdapterType,ImageType>)
      public:

        using value_type = typename ImageType::value_type;

        Base (const ImageType& parent) : parent_ (parent) { }


        template <class U> 
          const Base& operator= (const U& V) { return parent_ = V; }

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
        FORCE_INLINE const std::map<std::string, std::string>& keyval () const { return parent_.keyval(); }

        FORCE_INLINE ssize_t get_index (size_t axis) const { return parent_.index (axis); }
        FORCE_INLINE void move_index (size_t axis, ssize_t increment) { parent_.index (axis) += increment; }

        FORCE_INLINE value_type get_value () const { return parent_.value(); } 
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





