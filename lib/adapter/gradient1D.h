/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 22/02/12.

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

#ifndef __image_adapter_gradient1D_h__
#define __image_adapter_gradient1D_h__

#include "adapter/base.h"

namespace MR
{
  namespace Adapter
  {

    template <class ImageType>
      class Gradient1D : public Base<ImageType> {
      public:
        Gradient1D (const ImageType& parent,
                    size_t axis = 0) :
          Base<ImageType> (parent),
          axis (axis) { }

        typedef typename ImageType::value_type value_type;

        void set_axis (size_t val)
        {
          axis = val;
        }

        float& value ()
        {
          const ssize_t pos = index (axis);
          result = 0.0;

          if (pos == 0) {
            result = Base<ImageType>::value();
            index (axis) = pos + 1;
            result = Base<ImageType>::value() - result;
          } else if (pos == size(axis) - 1) {
            result = Base<ImageType>::value();
            index (axis) = pos - 1;
            result -= Base<ImageType>::value();
          } else {
            index (axis) = pos + 1;
            result = Base<ImageType>::value();
            index (axis) = pos - 1;
            result = 0.5 * (result - Base<ImageType>::value());
          }
          index (axis) = pos;

          return result;
        }

        using Base<ImageType>::name;
        using Base<ImageType>::size;
        using Base<ImageType>::spacing;
        using Base<ImageType>::index;

      protected:
        size_t axis;
        value_type result;
      };
  }
}


#endif

