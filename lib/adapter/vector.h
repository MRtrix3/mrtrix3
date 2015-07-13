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

#include "adapter/base.h"

namespace MR
{
  namespace Adapter
  {

    template <class ImageType>
      class Vector : public Base<ImageType> {

      public:
        typedef typename ImageType::value_type value_type;

        Vector (const ImageType& parent, size_t axis = 3, size_t vector_size = 0) :
          Base<ImageType> (parent), axis (axis), vector_size (vector_size) {
          if (!parent.is_direct_io())
            throw Exception ("Vector adapter class can only be used images loaded with direct IO access or scrach images");

          if (!vector_size) {
            vector_size = Base<ImageType>::size (axis);
          } else if (vector_size > parent.size (axis)) {
            throw Exception ("Vector adapter: vector size is larger than the size of the image along axis");
          }
          std::cout << vector_size << " " << axis << std::endl;
        }


        Eigen::Map<Eigen::Matrix<value_type, Eigen:: Dynamic, 1 > > value ()
        {
          vector_size = 3; // comment out this
          std::cout << vector_size <<  " " << axis << std::endl;
          Base<ImageType>::index (3) = 0;
          return Eigen::Map<Eigen::Matrix<value_type, Eigen:: Dynamic, 1 > > (Base<ImageType>::parent_.address(), vector_size);
        }

      protected:
        size_t axis;
        size_t vector_size;

      };

  }
}


#endif

