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

#ifndef __image_adapter_normalise3D_h__
#define __image_adapter_normalise3D_h__

#include "adapter/base.h"

namespace MR
{
    namespace Adapter 
    {


    template <class ImageType>
      class Normalise3D : 
        public Base<Normalise3D<ImageType>,ImageType> 
    { MEMALIGN(Normalise3D<ImageType>)
      public:

        typedef Base<Normalise3D<ImageType>,ImageType> base_type;
        typedef typename ImageType::value_type value_type;
        typedef Normalise3D voxel_type;

        using base_type::name;
        using base_type::size;
        using base_type::index;

        Normalise3D (const ImageType& parent) :
          base_type (parent) {
            set_extent (vector<int>(1,3));
          }

        Normalise3D (const ImageType& parent, const vector<int>& extent) :
          base_type (parent) {
            set_extent (extent);
          }

        void set_extent (const vector<int>& ext)
        {
          for (size_t i = 0; i < ext.size(); ++i)
            if (! (ext[i] & int(1)))
              throw Exception ("expected odd number for extent");
          if (ext.size() != 1 && ext.size() != 3)
            throw Exception ("unexpected number of elements specified in extent");
          if (ext.size() == 1)
            extent = vector<int> (3, ext[0]);
          else
            extent = ext;

          DEBUG ("normalise3D adapter for image \"" + name() + "\" initialised with extent " + str(extent));

          for (size_t i = 0; i < 3; ++i)
            extent[i] = (extent[i]-1)/2;
        }



        value_type value ()
        {
          const ssize_t old_pos [3] = { index(0), index(1), index(2) };
          pos_value = base_type::value();
          nelements = 0;

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

          mean = 0.0;

          for (index(2) = from[2]; index(2) < to[2]; ++index(2))
            for (index(1) = from[1]; index(1) < to[1]; ++index(1))
              for (index(0) = from[0]; index(0) < to[0]; ++index(0)){
                mean += base_type::value();
                nelements += 1;
              }
          mean /= nelements;

          index(0) = old_pos[0];
          index(1) = old_pos[1];
          index(2) = old_pos[2];

          return pos_value - mean;
        }

      protected:
        vector<int> extent;
        value_type mean;
        value_type pos_value;
        size_t nelements;
      };

  }
}


#endif

