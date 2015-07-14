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

#ifndef __image_adapter_vector_h__
#define __image_adapter_vector_h__

//#include "adapter/base.h"
#include "image.h"

namespace MR
{
  namespace Adapter
  {

    template <class ImageType>
      class Vector : public Image<typename ImageType::value_type> {

      public:
        typedef typename ImageType::value_type value_type;
        typedef Image<typename ImageType::value_type> ParentType;

        Vector (const ImageType& parent, ssize_t axis = -1, size_t size = 0) :
          ParentType (parent), vector_axis (axis), vector_size (size) {
          if (!parent.is_direct_io())
            throw Exception ("vector adapter can only be used images loaded with direct IO access or scrach images");

          if (vector_axis < 0)
            vector_axis = parent.ndim() - 1;
          else if (vector_axis >= parent.ndim())
            throw Exception ("axis requested in vector adapter is larger than the number of image dimensions");

          if (!vector_size)
            vector_size = parent.size (vector_axis);
          else if (vector_size > parent.size (vector_axis))
            throw Exception ("vector adapter: vector size is larger than the size of the image along axis");
        }


        Eigen::Map<Eigen::Matrix<value_type, Eigen::Dynamic, 1 >, Eigen::Unaligned, Eigen::InnerStride<> > value ()
        {
          index (vector_axis) = 0;
          return Eigen::Map<Eigen::Matrix<value_type, Eigen:: Dynamic, 1 >, Eigen::Unaligned, Eigen::InnerStride<> >
                   (address(), vector_size, Eigen::InnerStride<> (stride (vector_axis)));
        }

        using ParentType::index;
        using ParentType::size;
        using ParentType::address;
        using ParentType::stride;

      protected:
        ssize_t vector_axis;
        size_t vector_size;

      };

  }
}


#endif

