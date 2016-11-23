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

#ifndef __image_adapter_gradient3D_h__
#define __image_adapter_gradient3D_h__

#include "adapter/gradient1D.h"
#include "transform.h"

namespace MR
{
  namespace Adapter
  {

    template <class ImageType>
      class Gradient3D : public Gradient1D<ImageType> { MEMALIGN(Gradient3D<ImageType>)

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

