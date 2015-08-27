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

        typedef typename ImageType::value_type value_type;

        Gradient1D (const ImageType& parent,
                    size_t axis = 0,
                    bool wrt_spacing = false) :
          Base<ImageType> (parent),
          axis (axis),
          wrt_spacing (wrt_spacing),
          derivative_weights (3, 1.0),
          half_derivative_weights (3, 0.5)  
          {
            if (wrt_spacing) {
              for (size_t dim = 0; dim < 3; ++dim) {
                derivative_weights[dim] /= spacing(dim);
                half_derivative_weights[dim] /= spacing(dim);
              }
            }  
          }

        void set_axis (size_t val)
        {
          axis = val;
        }

        value_type value ()
        {
          const ssize_t pos = index (axis);
          result = 0.0;

          if (pos == 0) {
            result = Base<ImageType>::value();
            index (axis) = pos + 1;
            result = derivative_weights[axis] * (Base<ImageType>::value() - result);
          } else if (pos == size(axis) - 1) {
            result = Base<ImageType>::value();
            index (axis) = pos - 1;
            result = derivative_weights[axis] * (result - Base<ImageType>::value());
          } else {
            index (axis) = pos + 1;
            result = Base<ImageType>::value();
            index (axis) = pos - 1;
            result = half_derivative_weights[axis] * (result - Base<ImageType>::value());
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
        const bool wrt_spacing;
        std::vector<value_type> derivative_weights;
        std::vector<value_type> half_derivative_weights;
      };
  }
}


#endif

