/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __registration_transform_compose_h__
#define __registration_transform_compose_h__

#include "algo/threaded_loop.h"
#include "image.h"

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

    template <class WarpImageType>
      class ComposeKernel {
        public:
          ComposeKernel (const transform_type& transform, WarpImageType& warp_in, WarpImageType& warp_out) :
                           transform (transform.cast<typename WarpImageType::value_type>()), warp_in (warp_in), warp_out (warp_out) {}

          void operator() (const Iterator& index) {
            assign_pos_of (index, 0, 3).to (warp_out, warp_in);
            warp_out.row(3) = transform * warp_in.row(3).colwise().homogeneous();
          }

        protected:
          const Eigen::Transform<typename WarpImageType::value_type, 3, Eigen::AffineCompact> transform;
          WarpImageType warp_in;
          WarpImageType warp_out;
      };



      // Compose a 4x4 linear transform to each point in a deformation field. The input and output warp can be the same image.
      template <class WarpImageType>
      void compose (const transform_type& transform, WarpImageType& warp_in, WarpImageType& warp_out)
      {
        ThreadedLoop ("composing linear transform with warp", warp_in, 0, 3).run (ComposeKernel<WarpImageType> (transform, warp_in, warp_out));
      }

    }
  }
}

#endif
