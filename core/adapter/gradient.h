/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __image_adapter_gradient_h__
#define __image_adapter_gradient_h__

#include "adapter/base.h"

namespace MR
{
  namespace Adapter
  {

    template <class ImageType>
    class Gradient1D : public BaseFiniteDiff1D<Gradient1D<ImageType>, ImageType>
    { MEMALIGN (Gradient1D<ImageType>)
      public:

        using base_type = BaseFiniteDiff1D<Gradient1D<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;

        using base_type::index;
        using base_type::size;
        using base_type::axis;
        using base_type::axis_weights;



        Gradient1D (const ImageType& parent, size_t axis = 0, bool wrt_spacing = false) :
            base_type (parent, axis, wrt_spacing),
            half_axisweights (3, value_type(0.5))
        {
          if (wrt_spacing) {
            for (size_t dim = 0; dim < 3; ++dim)
              half_axisweights[dim] /= base_type::spacing (dim);
          }
        }

        /**
         * @brief computes the image gradient at the current index along the dimension defined by set_axis();
         * @return the image gradient
         */
        value_type value ()
        {
          const ssize_t pos = index (axis);
          value_type result = value_type(0);

          if (pos == 0) {
            result = base_type::value();
            index (axis) = pos + 1;
            result = axis_weights[axis] * (base_type::value() - result);
          } else if (pos == size(axis) - 1) {
            result = base_type::value();
            index (axis) = pos - 1;
            result = axis_weights[axis] * (result - base_type::value());
          } else {
            index (axis) = pos + 1;
            result = base_type::value();
            index (axis) = pos - 1;
            result = half_axisweights[axis] * (result - base_type::value());
          }
          index (axis) = pos;

          return result;
        }


      protected:
        vector<value_type> half_axisweights;
    };



    template <class ImageType>
    class Gradient3D : public BaseFiniteDiff3D<Gradient1D<ImageType>, ImageType>
    { MEMALIGN(Gradient3D<ImageType>)
      public:
        using base_type = BaseFiniteDiff3D<Gradient1D<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;
        using return_type = typename base_type::return_type;
        using base_type::base_type;
    };



  }
}


#endif

