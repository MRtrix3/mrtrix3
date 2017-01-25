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


#ifndef __image_adapter_gradient3D_h__
#define __image_adapter_gradient3D_h__

#include "adapter/gradient1D.h"
#include "transform.h"

namespace MR
{
  namespace Adapter
  {

    template <class ImageType>
      class Gradient3D : public Gradient1D<ImageType> {

      public:
        typedef typename ImageType::value_type value_type;

        Gradient3D (const ImageType& parent,
                    bool wrt_scanner = false) :
          Gradient1D<ImageType> (parent, wrt_scanner),
          wrt_scanner (wrt_scanner),
          transform (parent) {}

        Eigen::Matrix<value_type, 3, 1> value ()
        {
          Eigen::Matrix<value_type, 3, 1> grad;
          for (size_t i = 0; i < 3; ++i) {
            Gradient1D<ImageType>::set_axis(i);
            grad[i] = Gradient1D<ImageType>::value();
          }
          if (wrt_scanner)
            grad = transform.image2scanner.linear().template cast<value_type>() * grad;

          return grad;
        }

      protected:
        const bool wrt_scanner;
        Transform transform;
      };
  }
}

#endif

