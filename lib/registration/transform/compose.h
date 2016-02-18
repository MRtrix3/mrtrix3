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
              Eigen::Vector3 voxel ((default_type)disp_input1.index(0), (default_type)disp_input1.index(1), (default_type)disp_input1.index(2));
              Eigen::Vector3 voxel_position = disp1_transform.voxel2scanner * voxel;
              Eigen::Vector3 original_position = voxel_position + disp_input1.row(3);
              disp2_interp.scanner (original_position);
              if (!disp2_interp) {
                disp_output.row(3) = disp_input1.row(3);
              } else {
                Eigen::Vector3 displacement (disp2_interp.row(3).array() * step);
                Eigen::Vector3 new_position = displacement + original_position;
                disp_output.row(3) = new_position - voxel_position;
              }
            }

          protected:
            MR::Transform disp1_transform;
            Interp::Linear<Image<default_type> > disp2_interp;
            default_type step;
        };

        class ComposeHalfwayKernel {
          public:
            ComposeHalfwayKernel (const transform_type& linear1, Image<default_type>& disp1,
                                  Image<default_type>& disp2, const transform_type& linear2) :
                                    linear1 (linear1), disp1_interp (disp1), disp2_interp (disp2), linear2 (linear2) {
              out_of_bounds.setOnes();
              out_of_bounds *= NaN;
            }

            void operator() (Image<default_type>& deform) {
              Eigen::Vector3 voxel ((default_type)deform.index(0), (default_type)deform.index(1), (default_type)deform.index(2));
              Eigen::Vector3 position = linear1 * voxel;
              disp1_interp.scanner (position);
              if (!disp1_interp) {
                  deform.row(3) = out_of_bounds;
                } else {
                  Eigen::Vector3 midway_position = position + disp1_interp.row(3);
                  disp2_interp.scanner (midway_position);
                  if (!disp2_interp) {
                    deform.row(3) = out_of_bounds;
                  } else {
                    deform.row(3) = linear2 * (midway_position + disp2_interp.row(3));
                  }
               }
            }

          protected:
            const transform_type linear1;
            Interp::Linear<Image<default_type> > disp1_interp;
            Interp::Linear<Image<default_type> > disp2_interp;
            const transform_type linear2;
            Eigen::Vector3 out_of_bounds;
        };


      // Compose a linear transform and a deformation field. The input and output can be the same image.
      FORCE_INLINE void compose_affine_deformation (const transform_type& transform, Image<default_type>& deform_in, Image<default_type>& deform_out)
      {
        ThreadedLoop (deform_in, 0, 3).run (ComposeAffineDeformKernel (transform), deform_in, deform_out);
      }

      // Compose a linear transform and a displacement field. The output field is a deformation field. The input and output can be the same image.
      FORCE_INLINE  void compose_linear_displacement (const transform_type& transform, Image<default_type>& disp_in, Image<default_type>& deform_out)
      {
        ThreadedLoop (disp_in, 0, 3).run (ComposeLinearDispKernel (transform, disp_in), disp_in, deform_out);
      }

      // Compose two displacement fields and output a displacement field. The input and output can be the same image.
      FORCE_INLINE  void compose_displacement (Image<default_type>& disp_in1, Image<default_type>& disp_in2, Image<default_type>& disp_out, default_type step = 1.0)
      {
        ThreadedLoop (disp_in1, 0, 3).run (ComposeDispKernel (disp_in1, disp_in2, step), disp_in1, disp_out);
      }

      // Compose linear1<->displacement1<->[midway space]<->displacement2<->linear2. Output is a deformation field. TODO
      FORCE_INLINE void compose_halfway_transforms (const transform_type& linear1, Image<default_type>& disp1,
                                       Image<default_type>& disp2, const transform_type& linear2,
                                       Image<default_type>& deform_out)
      {
        MR::Transform deform_header_transform (deform_out);
        ThreadedLoop (deform_out, 0, 3).run (ComposeHalfwayKernel (linear1 * deform_header_transform.voxel2scanner, disp1, disp2, linear2), deform_out);
      }

    }
  }
}

#endif
