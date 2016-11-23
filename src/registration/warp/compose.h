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
#include "registration/warp/utils.h"
#include "adapter/extract.h"

namespace MR
{
  namespace Registration
  {
    namespace Warp
    {

        class ComposeLinearDeformKernel { MEMALIGN(ComposeLinearDeformKernel)
          public:
            ComposeLinearDeformKernel (const transform_type& transform) :
                                       transform (transform) {}


            template <class InputDeformationFieldType, class OutputDeformationFieldType>
            void operator() (InputDeformationFieldType& deform_input, OutputDeformationFieldType& deform_output) {
              deform_output.row(3) = transform * deform_input.row(3).colwise().homogeneous();
            }

          protected:
            const transform_type transform;
        };


        class ComposeLinearDispKernel { MEMALIGN(ComposeLinearDispKernel)
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

        class ComposeDispKernel { MEMALIGN(ComposeDispKernel)
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


        template <class DeformationField1Type, class DeformationField2Type>
        class ComposeHalfwayKernel { MEMALIGN(ComposeHalfwayKernel<DeformationField1Type,DeformationField2Type>)
          public:
            ComposeHalfwayKernel (const transform_type& linear1, DeformationField1Type& deform1,
                                  DeformationField2Type& deform2, const transform_type& linear2) :
                                    linear1 (linear1), deform1_interp (deform1), deform2_interp (deform2), linear2 (linear2) {
              out_of_bounds.setOnes();
              out_of_bounds *= NaN;
            }


            void operator() (Image<default_type>& deform) {
              Eigen::Vector3 voxel ((default_type)deform.index(0), (default_type)deform.index(1), (default_type)deform.index(2));
              Eigen::Vector3 position = linear1 * voxel;
              deform1_interp.scanner (position);
              if (!deform1_interp) {
                  deform.row(3) = out_of_bounds;
                } else {
                  Eigen::Vector3 position2 = deform1_interp.row(3);
                  deform2_interp.scanner (position2);
                  if (!deform2_interp) {
                    deform.row(3) = out_of_bounds;
                  } else {
                    Eigen::Vector3 position3 = deform2_interp.row(3);
                    deform.row(3) = linear2 * position3;
                  }
               }
            }

          protected:
            const transform_type linear1;
            Interp::Linear<DeformationField2Type> deform1_interp;
            Interp::Linear<DeformationField2Type> deform2_interp;
            const transform_type linear2;
            Eigen::Vector3 out_of_bounds;
        };


      // Compose a linear transform and a displacement field. The output field is a deformation field. The input and output can be the same image.
      template <class DisplacementFieldType, class DeformationFieldType>
      FORCE_INLINE  void compose_linear_displacement (const transform_type& transform, DisplacementFieldType& disp_in, DeformationFieldType& deform_out)
      {
        check_dimensions (disp_in, deform_out, 0, 3);
        ThreadedLoop (disp_in, 0, 3).run (ComposeLinearDispKernel (transform, disp_in), disp_in, deform_out);
      }

      // Compose a linear transform and a deformation field. The output field is a deformation field. The input and output can be the same image.
      template <class InputDeformationFieldType, class OutputDeformationFieldType>
      FORCE_INLINE  void compose_linear_deformation (const transform_type& transform, InputDeformationFieldType& deform_in, OutputDeformationFieldType& deform_out)
      {
        check_dimensions (deform_in, deform_out, 0, 3);
        ThreadedLoop (deform_in, 0, 3).run (ComposeLinearDeformKernel (transform), deform_in, deform_out);
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

        default_type scale_factor = 1.0;
        if (max_norm * step < min_vox_size / 2.0) {
          update_displacement (input, update, output, step);
        } else {
          scale_factor = std::pow (2, std::ceil (std::log ((max_norm * step) / (min_vox_size / 2.0)) / std::log (2.0)));

          std::shared_ptr<Image<default_type>> scaled_update = std::make_shared<Image<default_type> >(Image<default_type>::scratch (update));
          std::shared_ptr<Image<default_type>> composed = std::make_shared<Image<default_type> >(Image<default_type>::scratch (update));

          // Scaling
          default_type scaled_step = step / scale_factor; // apply the step size and scale factor at once
          ThreadedLoop (update).run (
                [&scaled_step](Image<default_type>& update, Image<default_type>& scaled_update) {
                  scaled_update.row(3) = update.row(3) * scaled_step;
                }, update, *scaled_update);

//          CONSOLE ("composing " + str(std::log2 (scale_factor)) + "times");

          // Squaring
          for (size_t i = 0; i < std::log2 (scale_factor); ++i) {
            update_displacement (*scaled_update, *scaled_update, *composed);
            std::swap (scaled_update, composed);
          }
//          save (*scaled_update, std::string("composed_update.mif"), false);
//          Adapter::Jacobian<Image<default_type> > jacobian (*scaled_update);
//          Header header (*scaled_update);
//          header.ndim() = 3;
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



      // Compose linear1<->deform1<->[midway space]<->deform2<->linear2.
      template <class DeformationField1Type, class DeformationField2Type, class OutputDeformationFieldType>
      FORCE_INLINE void compute_full_deformation (const transform_type& linear1, DeformationField1Type& deform1,
                                                  DeformationField2Type& deform2, const transform_type& linear2,
                                                  OutputDeformationFieldType& deform_out)
      {
        MR::Transform deform_header_transform (deform_out);
        ComposeHalfwayKernel<DeformationField1Type, DeformationField2Type> compose_kernel (linear1 * deform_header_transform.voxel2scanner, deform1, deform2, linear2);
        ThreadedLoop (deform_out, 0, 3).run (compose_kernel, deform_out);
      }

      // Compose linear1<->deform1<->[midway space]<->deform2<->linear2.
      template <class DeformationField1Type, class DeformationField2Type, class OutputDeformationFieldType>
      FORCE_INLINE void compute_full_deformation (std::string message, const transform_type& linear1, DeformationField1Type& deform1,
                                                  DeformationField2Type& deform2, const transform_type& linear2,
                                                  OutputDeformationFieldType& deform_out)
      {
        MR::Transform deform_header_transform (deform_out);
        ComposeHalfwayKernel<DeformationField1Type, DeformationField2Type> compose_kernel (linear1 * deform_header_transform.voxel2scanner, deform1, deform2, linear2);
        ThreadedLoop (message, deform_out, 0, 3).run (compose_kernel, deform_out);
      }

      template <class WarpType>
      FORCE_INLINE WarpType compute_midway_deformation (WarpType& warp, const int from) {
        Header midway_header (warp);
        midway_header.ndim() = 4;
        midway_header.size(3) = 3;
        WarpType deformation = WarpType::scratch (midway_header);

        transform_type linear;
        std::vector<int> index(1);
        if (from == 1) {
          linear = Registration::Warp::parse_linear_transform (warp, "linear1");
          index[0] = 0;
        } else {
          linear = Registration::Warp::parse_linear_transform (warp, "linear2");
          index[0] = 2;
        }
        Adapter::Extract1D<WarpType> im_to_mid (warp, 4, index);
        Registration::Warp::compose_linear_deformation (linear, im_to_mid, deformation);
        return deformation;
      }

      template <class WarpType, class TemplateType>
      FORCE_INLINE WarpType compute_full_deformation (WarpType& warp, TemplateType& template_image, const int from) {
        Header deform_header (template_image);
        deform_header.ndim() = 4;
        deform_header.size(3) = 3;
        WarpType deform = WarpType::scratch (deform_header);

        transform_type linear1 = Registration::Warp::parse_linear_transform (warp, "linear1");
        transform_type linear2 = Registration::Warp::parse_linear_transform (warp, "linear2");

        std::vector<int> index(1);
        if (from == 1) {
          index[0] = 0;
          Adapter::Extract1D<Image<default_type>> im1_to_mid (warp, 4, index);
          index[0] = 3;
          Adapter::Extract1D<Image<default_type>> mid_to_im2 (warp, 4, index);
          Registration::Warp::compute_full_deformation (linear2.inverse(), mid_to_im2, im1_to_mid, linear1, deform);
        } else {
          index[0] = 1;
          Adapter::Extract1D<Image<default_type>> mid_to_im1 (warp, 4, index);
          index[0] = 2;
          Adapter::Extract1D<Image<default_type>> im2_to_mid (warp, 4, index);
          Registration::Warp::compute_full_deformation (linear1.inverse(), mid_to_im1, im2_to_mid, linear2, deform);
        }
        return deform;
      }

    }
  }
}

#endif
