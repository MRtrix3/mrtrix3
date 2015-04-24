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

#include "math/matrix.h"
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
            parent (parent) {
            }

          typedef typename ImageType::value_type value_type;
          typedef Base voxel_type; // TODO why is this needed...?

          template <class U> 
            const Base& operator= (const U& V) { return parent = V; }

          const Header& header () const { return parent.header(); }
          const std::string& name () const { return parent.name(); }
          size_t ndim () const { return parent.ndim(); }
          ssize_t size (size_t axis) const { return parent.size (axis); }
          default_type voxsize (size_t axis) const { return parent.voxsize (axis); }
          ssize_t stride (size_t axis) const { return parent.stride (axis); }
          const Math::Matrix<default_type>& transform () const { return parent.transform(); }

          ssize_t index (size_t axis) const { return get_voxel_position (axis); }
          auto index (size_t axis) -> decltype(Helper::voxel_index(*this, axis)) { return { *this, axis }; }

          value_type value () const { return get_voxel_value (); }
          auto value () -> decltype(Helper::voxel_value(*this)) { return { *this }; }

          void reset () { parent.reset(); }

          friend std::ostream& operator<< (std::ostream& stream, const Base& V) {
            stream << "image adapter \"" << V.name() << "\", datatype " << MR::DataType::from<value_type>().specifier() << ", position [ ";
            for (size_t n = 0; n < V.ndim(); ++n) 
              stream << V[n] << " ";
            stream << "], value = " << V.value();
            return stream;
          }

        protected:
          ImageType parent;

          value_type get_voxel_value () const { return parent.value(); } 
          void set_voxel_value (value_type val) { parent.value() = val; } 

          ssize_t get_voxel_position (size_t axis) const { return parent.index (axis); }
          void set_voxel_position (size_t axis, ssize_t position) { parent.index (axis) = position; }
          void move_voxel_position (size_t axis, ssize_t increment) { parent.index (axis) += increment; }

          friend class Helper::VoxelIndex<Base>;
          friend class Helper::VoxelValue<Base>;
      };


  }
}

#endif





