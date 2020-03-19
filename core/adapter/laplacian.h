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

#ifndef __image_adapter_laplacian_h__
#define __image_adapter_laplacian_h__

#include "types.h"
#include "adapter/base.h"


namespace MR
{
  namespace Adapter
  {


    template <class ImageType>
    class Laplacian1D : public BaseFiniteDiff1D<Laplacian1D<ImageType>, ImageType>
    { MEMALIGN (Laplacian1D<ImageType>)
      public:

        using base_type = BaseFiniteDiff1D<Laplacian1D<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;

        using base_type::index;
        using base_type::size;

        using base_type::axis;
        using base_type::axis_weights;



        Laplacian1D (const ImageType& parent, size_t axis = 0, bool wrt_spacing = false) :
            base_type (parent, axis, wrt_spacing) { }

        /**
         * @brief computes the image Laplacian (second derivative) at the current index
         * along the dimension defined by set_axis();
         * @return the image Laplacian
         */
        value_type value ()
        {
          const ssize_t pos = index (axis);
          if (pos == 0 || pos == size(axis) - 1)
            return value_type(0);

          value_type result = value_type(-2.0) * base_type::value();
          index (axis) = pos - 1;
          result += base_type::value();
          index (axis) = pos + 1;
          result = axis_weights[axis] * (result + base_type::value());
          index (axis) = pos;
          return result;
        }

    };



    template <class ImageType>
    class Laplacian3D : public BaseFiniteDiff3D<Laplacian1D<ImageType>, ImageType>
    { MEMALIGN(Laplacian3D<ImageType>)
      public:
        using base_type = BaseFiniteDiff3D<Laplacian1D<ImageType>, ImageType>;
        using value_type = typename ImageType::value_type;
        using return_type = typename base_type::return_type;
        using base_type::base_type;
    };





  }
}


#endif

