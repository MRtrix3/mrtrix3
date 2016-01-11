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
#include "interp/linear.h"

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

        class ComposeAffineDeformKernel {
          public:
            ComposeAffineDeformKernel (const transform_type& transform) :
                                       transform (transform) {}

            void operator() (Image<default_type>& deform_input, Image<default_type>& deform_output) {
              deform_output.row(3) = transform * deform_input.row(3).colwise().homogeneous();
            }

          protected:
            const transform_type transform;
        };


        class ComposeLinearDispKernel {
          public:
            ComposeLinearDispKernel (const transform_type& transform, const Image<default_type>& warp_in) :
                                     transform (transform),
                                     image_transform (warp_in) {}

            void operator() (Image<default_type>& disp_input, Image<default_type>& disp_output) {
              Eigen::Vector3 voxel (disp_input.index(0), disp_input.index(1), disp_input.index(2));
              disp_output.row(3) = transform * (image_transform.voxel2scanner * voxel + disp_input.row(3));
            }

          protected:
            const transform_type transform;
            MR::Transform image_transform;
        };

        class ComposeDispKernel {
          public:
            ComposeDispKernel (Image<default_type>& disp_input1, Image<default_type>& disp_input2, default_type step) :
                               disp1_transform (disp_input1), disp2_interp (disp_input2), step (step) {}

            void operator() (Image<default_type>& disp_input1, Image<default_type>& disp_output) {
              Eigen::Vector3 voxel (disp_input1.index(0), disp_input1.index(1), disp_input1.index(2));
              Eigen::Vector3 voxel_position = disp1_transform.voxel2scanner * voxel;
              Eigen::Vector3 original_position = voxel_position + disp_input1.row(3);
              Eigen::Vector3 new_position (original_position);
              disp2_interp.scanner (original_position);
              if (!disp2_interp) {
                Eigen::Vector3 displacement (disp2_interp.row(3).array() * step);
                new_position = displacement + original_position;
              }
              disp_output.row(3) = new_position - voxel_position;
            }

          protected:
            MR::Transform disp1_transform;
            Interp::Linear<Image<default_type> > disp2_interp;
            default_type step;
        };


      // Compose a linear transform and a deformation field. The input and output can be the same image.
      void compose_affine_deformation (const transform_type& transform, Image<default_type>& deform_in, Image<default_type>& deform_out)
      {
        ThreadedLoop ("composing linear transform with warp...", deform_in, 0, 3).run (ComposeAffineDeformKernel (transform), deform_in, deform_out);
      }

      // Compose a linear transform and a displacement field. The output field is a deformation field. The input and output can be the same image.
      void compose_affine_displacement (const transform_type& transform, Image<default_type>& disp_in, Image<default_type>& deform_out)
      {
        ThreadedLoop ("composing linear transform with warp...", disp_in, 0, 3).run (ComposeLinearDispKernel (transform, disp_in), disp_in, deform_out);
      }

      // Compose two displacement fields and output a displacement field. The input and output can be the same image.
      void compose_displacement (Image<default_type>& disp_in1, Image<default_type>& disp_in2, Image<default_type>& disp_out, default_type step = 1.0)
      {
        ThreadedLoop (disp_in1, 0, 3).run (ComposeDispKernel (disp_in1, disp_in2, step), disp_in1, disp_out);
      }

    }
  }
}

#endif
