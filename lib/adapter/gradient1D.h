/* Copyright (c) 2008-2017 the MRtrix3 contributors
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

        /**
         * @brief computes the image gradient at the current index along the dimension defined by set_axis();
         * @return the image gradient
         */
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

