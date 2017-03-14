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


#ifndef __registration_metric_params_h__
#define __registration_metric_params_h__

#include "image.h"
#include "interp/linear.h"
#include "interp/nearest.h"
#include "adapter/reslice.h"
#include "registration/multi_contrast.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {

      template <class TransformType,
                class Im1ImageType,
                class Im2ImageType,
                class MidwayImageType,
                class Im1MaskType,
                class Im2MaskType,
                class Im1ImageInterpType,
                class Im2ImageInterpType,
                class Im1MaskInterpolatorType,
                class Im2MaskInterpolatorType,
                class ProcImageType = Image<default_type>,
                class ProcImageInterpolatorType = Interp::Linear<Image<default_type>>,
                class ProcMaskType = Image<bool>,
                class ProcessedMaskInterpolatorType = Interp::Nearest<Image<bool>>>
      class Params {
        MEMALIGN (Params<TransformType,Im1ImageType,Im2ImageType,MidwayImageType,
            Im1MaskType,Im2MaskType,Im1ImageInterpType,Im2ImageInterpType,
            Im1MaskInterpolatorType,Im2MaskInterpolatorType,ProcImageType,ProcImageInterpolatorType,
            ProcMaskType,ProcessedMaskInterpolatorType>)
        public:

          typedef typename TransformType::ParameterType TransformParamType;
          typedef typename Im1ImageInterpType::value_type Im1ValueType;
          typedef typename Im2ImageInterpType::value_type Im2ValueType;
          typedef Im1ImageInterpType Im1InterpType;
          typedef Im2ImageInterpType Im2InterpType;
          typedef Im1MaskInterpolatorType Mask1InterpolatorType;
          typedef Im2MaskInterpolatorType Mask2InterpolatorType;
          typedef typename ProcImageInterpolatorType::value_type ProcessedValueType;
          typedef ProcImageType ProcessedImageType;
          typedef ProcMaskType ProcessedMaskType;
          typedef ProcImageInterpolatorType ProcessedImageInterpType;
          typedef ProcessedMaskInterpolatorType ProcessedMaskInterpType;

          Params (TransformType& transform,
                  Im1ImageType& im1_image,
                  Im2ImageType& im2_image,
                  MidwayImageType& midway_image,
                  Im1MaskType& im1_mask,
                  Im2MaskType& im2_mask) :
                    transformation (transform),
                    im1_image (im1_image),
                    im2_image (im2_image),
                    midway_image (midway_image),
                    im1_mask (im1_mask),
                    im2_mask (im2_mask),
                    loop_density (1.0),
                    robust_estimate (false),
                    control_point_exent (10.0, 10.0, 10.0) {
                      im1_image_interp.reset (new Im1ImageInterpType (im1_image));
                      im2_image_interp.reset (new Im2ImageInterpType (im2_image));
                      if (im1_mask.valid())
                        im1_mask_interp.reset (new Im1MaskInterpolatorType (im1_mask));
                      if (im2_mask.valid())
                        im2_mask_interp.reset (new Im2MaskInterpolatorType (im2_mask));
                      update_control_points();
          }

          void set_extent (vector<size_t> extent_vector) { extent=std::move(extent_vector); }

          void set_mc_settings (const vector<MultiContrastSetting>& mc_vector) {
            DEBUG ("setting mc_settings");
            mc_settings = mc_vector;
          }

          template <class VectorType>
          void set_control_points_extent(const VectorType& extent) {
            control_point_exent = extent;
            update_control_points();
          }

          void set_im1_iterpolator (Im1ImageType& im1_image) {
            im1_image_interp.reset (new Im1ImageInterpType (im1_image));
          }
          void set_im2_iterpolator (Im1ImageType& im2_image) {
            im2_image_interp.reset (new Im2ImageInterpType (im2_image));
          }

          void update_control_points () {
            const Eigen::Vector3 centre = transformation.get_centre();
            control_points.resize(4, 4);
            // tetrahedron centred at centre of midspace scaled by control_point_exent
            control_points <<  1.0, -1.0, -1.0,  1.0,
                               1.0,  1.0, -1.0, -1.0,
                               1.0, -1.0,  1.0, -1.0,
                               1.0,  1.0,  1.0,  1.0;
            for (size_t i = 0; i < 3; ++i)
              control_points.row(i) *= control_point_exent[i];
            control_points.block<3,4>(0,0).colwise() += centre;
          }

          const vector<size_t>& get_extent() const { return extent; }

          template <class OptimiserType>
            void optimiser_update (OptimiserType& optim, const ssize_t overlap_count) {
              DEBUG ("gradient descent ran using " + str(optim.function_evaluations()) + " cost function evaluations.");
              if (!is_finite(optim.state())) {
                CONSOLE ("last valid transformation:");
                transformation.debug();
                CONSOLE ("last optimisation step ran using " + str(optim.function_evaluations()) + " cost function evaluations.");
                if (overlap_count == 0)
                  WARN ("linear registration failed because (masked) images do not overlap.");
                throw Exception ("Linear registration failed, transformation parameters are NaN.");
              }
              transformation.set_parameter_vector (optim.state());
              update_control_points();
            }

          void make_diagnostics_image (const std::basic_string<char>& image_path, bool masked = true) {
              Header header (midway_image);
              header.datatype() = DataType::Float64;
              header.ndim() = 4;
              header.size(3) = 3;
              // auto check = Image<float>::scratch (header);
              auto check = Image<default_type>::create (image_path, header);

              auto trafo1 = transformation.get_transform_half();
              auto trafo2 = transformation.get_transform_half_inverse();

              Adapter::Reslice<Interp::Linear, Im1ImageType > im1_reslicer (
                im1_image, midway_image, trafo1, Adapter::AutoOverSample, NAN);
              Adapter::Reslice<Interp::Linear, Im2ImageType > im2_reslicer (
                im2_image, midway_image, trafo2, Adapter::AutoOverSample, NAN);

              auto T = MR::Transform(midway_image).voxel2scanner;
              Eigen::Vector3 midway_point, voxel_pos, im1_point, im2_point;

              for (auto i = Loop (midway_image) (check, im1_reslicer, im2_reslicer); i; ++i) {
                voxel_pos = Eigen::Vector3 ((default_type)check.index(0), (default_type)check.index(1), (default_type)check.index(2));
                midway_point = T * voxel_pos;

                check.index(3) = 0;
                check.value() = im1_reslicer.value();
                if (masked and im1_mask_interp) {
                  transformation.transform_half (im1_point, midway_point);
                  im1_mask_interp->scanner (im1_point);
                  if (im1_mask_interp->value() <= 0.5)
                    check.value() = NAN;
                }

                check.index(3) = 1;
                check.value() = im2_reslicer.value();
                if (masked and im2_mask_interp) {
                  transformation.transform_half_inverse (im2_point, midway_point);
                  im2_mask_interp->scanner (im1_point);
                  if (im2_mask_interp->value() <= 0.5)
                    check.value() = NAN;
                }
                check.index(3) = 0;
              }
              INFO("diagnostics image written");
              // display (check);
          }

          TransformType& transformation;
          Im1ImageType im1_image;
          Im2ImageType im2_image;
          MidwayImageType midway_image;
          MR::copy_ptr<Im1ImageInterpType> im1_image_interp;
          MR::copy_ptr<Im2ImageInterpType> im2_image_interp;
          Im1MaskType im1_mask;
          Im2MaskType im2_mask;
          MR::copy_ptr<Im1MaskInterpolatorType> im1_mask_interp;
          MR::copy_ptr<Im2MaskInterpolatorType> im2_mask_interp;
          default_type loop_density;
          bool robust_estimate;
          Eigen::Vector3 control_point_exent;
          Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> control_points;
          vector<size_t> extent;
          vector<MultiContrastSetting> mc_settings;

          ProcImageType processed_image;
          MR::copy_ptr<ProcImageInterpolatorType> processed_image_interp;
          ProcMaskType processed_mask;
          MR::copy_ptr<ProcessedMaskInterpolatorType> processed_mask_interp;
      };
    }
  }
}

#endif
