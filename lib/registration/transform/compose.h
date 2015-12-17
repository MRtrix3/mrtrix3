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
#include "transform.h"

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

    template <class WarpImageType>
      class ComposeDeformKernel {
        public:
          ComposeDeformKernel (const transform_type& transform) :
                               transform (transform.cast<typename WarpImageType::value_type>()) {}

          void operator() (WarpImageType& warp_in, WarpImageType& warp_out) {
            warp_out.row(3) = transform * warp_in.row(3).colwise().homogeneous();
          }

        protected:
          const Eigen::Transform<typename WarpImageType::value_type, 3, Eigen::AffineCompact> transform;
      };


      template <class WarpImageType>
        class ComposeDispKernel {
          public:
            ComposeDispKernel (const transform_type& transform, const WarpImageType& warp_in) :
                               transform (transform.cast<typename WarpImageType::value_type>()),
                               image_transform (warp_in) {}

            void operator() (WarpImageType& warp_in, WarpImageType& warp_out) {
              Eigen::Vector3 voxel ((default_type)warp_in.index(0), (default_type)warp_in.index(1), (default_type)warp_in.index(2));
              warp_out.row(3) = transform * ((image_transform.voxel2scanner * voxel).template cast<typename WarpImageType::value_type> () + warp_in.row(3));
            }

          protected:
            const Eigen::Transform<typename WarpImageType::value_type, 3, Eigen::AffineCompact> transform;
            MR::Transform image_transform;
        };


      // Compose a linear transform and a deformation field. The input and output can be the same image.
      template <class WarpImageType>
      void compose_affine_deformation (const transform_type& transform, WarpImageType& deform_in, WarpImageType& deform_out)
      {
        ThreadedLoop ("composing linear transform with warp...", deform_in, 0, 3).run (ComposeDeformKernel<WarpImageType> (transform), deform_in, deform_out);
      }

      // Compose a linear transform and a displacement field. The output field is a deformation field. The input and output can be the same image.
      template <class WarpImageType>
      void compose_affine_displacement (const transform_type& transform, WarpImageType& disp_in, WarpImageType& deform_out)
      {
        ThreadedLoop ("composing linear transform with warp...", disp_in, 0, 3).run (ComposeDispKernel<WarpImageType> (transform, disp_in), disp_in, deform_out);
      }

    }
  }
}

#endif
