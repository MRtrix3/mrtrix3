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

#ifndef __registration_metric_params_h__
#define __registration_metric_params_h__

#include "image.h"
#include "interp/linear.h"
#include "interp/nearest.h"

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
        MEMALIGN(Params<TransformType,Im1ImageType,Im2ImageType,MidwayImageType, 
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

          ProcImageType processed_image;
          MR::copy_ptr<ProcImageInterpolatorType> processed_image_interp;
          ProcMaskType processed_mask;
          MR::copy_ptr<ProcessedMaskInterpolatorType> processed_mask_interp;
      };
    }
  }
}

#endif
