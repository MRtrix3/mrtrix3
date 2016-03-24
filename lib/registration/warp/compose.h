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

#ifndef __registration_warp_compose_h__
#define __registration_warp_compose_h__

#include "algo/threaded_loop.h"
#include "image.h"
#include "transform.h"
#include "interp/linear.h"
#include "adapter/jacobian.h" //TODO remove after debug

namespace MR
{
  namespace Registration
  {
    namespace Warp
    {

        class ComposeLinearDeformKernel {
          public:
            ComposeLinearDeformKernel (const transform_type& transform) :
                                       transform (transform) {}

            void operator() (Image<default_type>& deform_input, Image<default_type>& deform_output) {
              deform_output.row(3) = transform * deform_input.row(3).colwise().homogeneous();
            }

          protected:
            const transform_type transform;
        };


        class ComposeLinearDispKernel {
          public:
            template<class DisplacementFieldType>
            ComposeLinearDispKernel (const transform_type& linear_transform, const DisplacementFieldType& disp_in) :
                                     linear_transform (linear_transform),
                                     image_transform (disp_in) {}

            template <class DisplacementFieldType, class DeformationFieldType>
            void operator() (DisplacementFieldType& disp_input, DeformationFieldType& deform_output) {
              Eigen::Vector3 voxel (disp_input.index(0), disp_input.index(1), disp_input.index(2));
              deform_output.row(3) = linear_transform * (image_transform.voxel2scanner * voxel + disp_input.row(3));
            }

          protected:
            const transform_type linear_transform;
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


        template <class DisplacementField1Type, class DisplacementField2Type>
        class ComposeHalfwayKernel {
          public:
            ComposeHalfwayKernel (const transform_type& linear1, DisplacementField1Type& disp1,
                                  DisplacementField2Type& disp2, const transform_type& linear2) :
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
            Interp::Linear<DisplacementField2Type> disp1_interp;
            Interp::Linear<DisplacementField2Type> disp2_interp;
            const transform_type linear2;
            Eigen::Vector3 out_of_bounds;
        };


      // Compose a linear transform and a deformation field. The input and output can be the same image.
      FORCE_INLINE void compose_linear_deformation (const transform_type& transform, Image<default_type>& deform_in, Image<default_type>& deform_out)
      {
        check_dimensions (deform_in, deform_out, 0, 3);
        ThreadedLoop (deform_in, 0, 3).run (ComposeLinearDeformKernel (transform), deform_in, deform_out);
      }

      // Compose a linear transform and a displacement field. The output field is a deformation field. The input and output can be the same image.
      template <class DisplacementFieldType, class DeformationFieldType>
      FORCE_INLINE  void compose_linear_displacement (const transform_type& transform, DisplacementFieldType& disp_in, DeformationFieldType& deform_out)
      {
        check_dimensions (disp_in, deform_out, 0, 3);
        ThreadedLoop (disp_in, 0, 3).run (ComposeLinearDispKernel (transform, disp_in), disp_in, deform_out);
      }

      // Compose two displacement fields and output a displacement field. The input and output can be the same image.
      FORCE_INLINE  void update_displacement (Image<default_type>& input, Image<default_type>& update, Image<default_type>& output, default_type step = 1.0)
      {
        check_dimensions (input, output, 0, 3);
        ThreadedLoop (input, 0, 3).run (ComposeDispKernel (input, update, step), input, output);
      }

      // Compose two displacement fields and output a displacement field using scaling and squaring.  The input and output can be the same image.
      FORCE_INLINE  void update_displacement_scaling_and_squaring (Image<default_type>& input, Image<default_type>& update, Image<default_type>& output, const default_type step = 1.0)
      {
        check_dimensions (input, output, 0, 3);

        default_type max_norm = 0.0;
        auto max_norm_func = [&max_norm](Image<default_type>& update) {
          default_type norm = update.row(3).norm();
          if (norm > max_norm)
            max_norm = norm;
        };
        ThreadedLoop (update).run (max_norm_func, update);
        default_type min_vox_size = static_cast<default_type> (std::min (input.spacing(0), std::min (input.spacing(1), input.spacing(2))));

        // if the maximum update is larger than half a voxel, perform scaling and squaring to ensure the displacement field remains diffeomorphic
        if (max_norm * step < min_vox_size / 2.0) {
          update_displacement (input, update, output, step);
        } else {
          default_type scale_factor = std::pow (2, std::ceil (std::log ((max_norm * step) / (min_vox_size / 2.0)) / std::log (2.0)));
          scale_factor = 32;
//          CONSOLE ("scaling and squaring: " + str(scale_factor));
          std::shared_ptr<Image<default_type>> scaled_update = std::make_shared<Image<default_type> >(Image<default_type>::scratch (update));
          std::shared_ptr<Image<default_type>> composed = std::make_shared<Image<default_type> >(Image<default_type>::scratch (update));
          default_type temp = step / scale_factor; // apply the step size and scale factor at once
          ThreadedLoop (update).run (
                [&temp](Image<default_type>& update, Image<default_type>& scaled_update) {
                  scaled_update.row(3) = update.row(3) * temp;
                }, update, *scaled_update);

//          CONSOLE ("composing " + str(std::log2 (scale_factor)) + "times");
          for (size_t i = 0; i < std::log2 (scale_factor); ++i) {
            update_displacement (*scaled_update, *scaled_update, *composed);
            std::swap (scaled_update, composed);
          }
//          save (*scaled_update, std::string("composed_update.mif"), false);
//          Adapter::Jacobian<Image<default_type> > jacobian (*scaled_update);
//          Header header (*scaled_update);
//          header.set_ndim(3);
//          bool is_neg = false;
//          auto jacobian_det = Image<default_type>::scratch (header);
//          Eigen::MatrixXd ident = Eigen::MatrixXd::Identity (3,3);
//          for (auto i = Loop (0,3) (jacobian, jacobian_det); i; ++i) {
//            auto jac_matrix = ident + jacobian.value();
//            jacobian_det.value() = jac_matrix.determinant();
//            if (jacobian_det.value() < 0.0)
//              is_neg = true;
//          }
//          save (jacobian_det, std::string("jacobian.mif"), false);
//          if (is_neg)
//            throw Exception ("negative jacobians in update");

          update_displacement (input, *scaled_update, output);
        }
      }

      // Compose linear1<->displacement1<->[midway space]<->displacement2<->linear2. Output is a deformation field.
      template <class DisplacementField1Type, class DisplacementField2Type, class DeformationFieldType>
      FORCE_INLINE void compose_halfway_transforms (const transform_type& linear1, DisplacementField1Type& disp1,
                                                    DisplacementField2Type& disp2, const transform_type& linear2,
                                                    DeformationFieldType& deform_out)
      {
        MR::Transform deform_header_transform (deform_out);
        ComposeHalfwayKernel<DisplacementField1Type, DisplacementField2Type> compose_kernel (linear1 * deform_header_transform.voxel2scanner, disp1, disp2, linear2);
        ThreadedLoop (deform_out, 0, 3).run (compose_kernel, deform_out);
      }

      // Compose linear1<->displacement1<->[midway space]<->displacement2<->linear2. Output is a deformation field.
      template <class DisplacementField1Type, class DisplacementField2Type, class DeformationFieldType>
      FORCE_INLINE void compose_halfway_transforms (std::string message, const transform_type& linear1, DisplacementField1Type& disp1,
                                                    DisplacementField2Type& disp2, const transform_type& linear2,
                                                    DeformationFieldType& deform_out)
      {
        MR::Transform deform_header_transform (deform_out);
        ComposeHalfwayKernel<DisplacementField1Type, DisplacementField2Type> compose_kernel (linear1 * deform_header_transform.voxel2scanner, disp1, disp2, linear2);
        ThreadedLoop (message, deform_out, 0, 3).run (compose_kernel, deform_out);
      }

    }
  }
}

#endif
