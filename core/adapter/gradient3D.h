/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#ifndef __image_adapter_gradient3D_h__
#define __image_adapter_gradient3D_h__

#include "adapter/gradient1D.h"
#include "transform.h"

namespace MR
{
  namespace Adapter
  {

    template <class ImageType>
      class Gradient3D : 
        public Gradient1D<ImageType> 
    { MEMALIGN(Gradient3D<ImageType>)

      public:

        using value_type = Eigen::Matrix<typename ImageType::value_type,3,1>;

        Gradient3D (const ImageType& parent,
                    bool wrt_scanner = false) :
          Gradient1D<ImageType> (parent, wrt_scanner),
          wrt_scanner (wrt_scanner),
          transform (parent) {}

        value_type value ()
        {
          value_type grad;
          for (size_t i = 0; i < 3; ++i) {
            Gradient1D<ImageType>::set_axis(i);
            grad[i] = Gradient1D<ImageType>::value();
          }
          if (wrt_scanner)
            grad = transform.image2scanner.linear().template cast<typename ImageType::value_type>() * grad;

          return grad;
        }

      protected:
        const bool wrt_scanner;
        Transform transform;
      };
  }
}

#endif

