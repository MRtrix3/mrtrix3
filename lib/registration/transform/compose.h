/*
    Copyright 2015 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 2015

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
          const Eigen::Transform<typename WarpImageType::value_type, 3, Eigen::AffineCompact> & transform;
          WarpImageType warp_in;
          WarpImageType warp_out;
      };



      // Compose a 4x4 linear transform to each point in a deformation field. The input and output warp can be the same image.
      template <class WarpImageType>
      void compose (const transform_type& transform, WarpImageType& warp_in, WarpImageType& warp_out)
      {
        ThreadedLoop ("composing", warp_in, 0, 3).run (ComposeKernel<WarpImageType> (transform, warp_in, warp_out));
      }

    }
  }
}

#endif
