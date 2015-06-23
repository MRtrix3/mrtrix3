/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __image_adapter_median3D_h__
#define __image_adapter_median3D_h__

#include "math/median.h"
#include "adapter/base.h"

namespace MR
{
    namespace Adapter 
    {


    template <class ImageType>
      class Median3D : public Base<ImageType> {
      public:
        Median3D (const ImageType& parent) :
          Base<ImageType> (parent) {
            set_extent (std::vector<int>(1,3));
          }

        Median3D (const ImageType& parent, const std::vector<int>& extent) :
          Base<ImageType> (parent) {
            set_extent (extent);
          }

        typedef typename ImageType::value_type value_type;
        typedef Median3D voxel_type;

        void set_extent (const std::vector<int>& ext)
        {
          for (size_t i = 0; i < ext.size(); ++i)
            if (! (ext[i] & int(1)))
              throw Exception ("expected odd number for extent");
          if (ext.size() != 1 && ext.size() != 3)
            throw Exception ("unexpected number of elements specified in extent");
          if (ext.size() == 1)
            extent = std::vector<int> (3, ext[0]);
          else
            extent = ext;

          DEBUG ("median3D adapter for image \"" + name() + "\" initialised with extent " + str(extent));

          for (size_t i = 0; i < 3; ++i)
            extent[i] = (extent[i]-1)/2;
        }



        value_type& value ()
        {
          const ssize_t old_pos [3] = { index(0), index(1), index(2) };
          const ssize_t from[3] = {
            index(0) < extent[0] ? 0 : index(0) - extent[0],
            index(1) < extent[1] ? 0 : index(1) - extent[1],
            index(2) < extent[2] ? 0 : index(2) - extent[2]
          };
          const ssize_t to[3] = {
            index(0) >= size(0)-extent[0] ? size(0) : index(0)+extent[0]+1,
            index(1) >= size(1)-extent[1] ? size(1) : index(1)+extent[1]+1,
            index(2) >= size(2)-extent[2] ? size(2) : index(2)+extent[2]+1
          };

          values.clear();

          for (index(2) = from[2]; index(2) < to[2]; ++index(2))
            for (index(1) = from[1]; index(1) < to[1]; ++index(1))
              for (index(0) = from[0]; index(0) < to[0]; ++index(0))
                values.push_back (Base<ImageType>::value());

          index(0) = old_pos[0];
          index(1) = old_pos[1];
          index(2) = old_pos[2];

          retval = Math::median (values);
          return retval;
        }

        using Base<ImageType>::name;
        using Base<ImageType>::size;
        using Base<ImageType>::index;

      protected:
        std::vector<int> extent;
        std::vector<value_type> values;
        value_type retval;
      };

  }
}


#endif

