/*
   Copyright 2012 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt, 24/02/2012

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

#ifndef __registration_metric_syn_demons_h__
#define __registration_metric_syn_demons_h__

#include "adapter/gradient3D.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {

      template <class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
      class SyNDemons {
        public:
          SyNDemons (default_type& global_energy, size_t& global_voxel_count,
                     const Im1ImageType& im1_image, const Im2ImageType& im2_image, const Im1MaskType im1_mask, const Im2MaskType im2_mask) :
                       global_cost (global_energy),
                       global_voxel_count (global_voxel_count),
                       thread_cost (0.0),
                       thread_voxel_count (0),
                       normaliser (0.0),
                       robustness_parameter (-1.e12),
                       intensity_difference_threshold (0.001),
                       denominator_threshold (1e-9),
                       im1_gradient (im1_image, true), im2_gradient (im2_image, true),
                       im1_mask (im1_mask), im2_mask (im2_mask)
          {
            for (size_t d = 0; d < 3; ++d)
              normaliser += im1_image.spacing(d) * im2_image.spacing(d);
            normaliser /= 3.0;
          }

          ~SyNDemons () {
            global_cost += thread_cost;
            global_voxel_count += thread_voxel_count;
          }

          void set_im1_mask (const Image<float>& mask) {
            im1_mask = mask;
          }

          void set_im2_mask (const Image<float>& mask) {
            im2_mask = mask;
          }


          void operator() (const Im1ImageType& im1_image,
                           const Im2ImageType& im2_image,
                           Image<default_type>& im1_update,
                           Image<default_type>& im2_update) {

            // Check for boundary condition
            if (im1_image.index(0) == 0 || im1_image.index(0) == im1_image.size(0) - 1 ||
                im1_image.index(1) == 0 || im1_image.index(1) == im1_image.size(1) - 1 ||
                im1_image.index(2) == 0 || im1_image.index(2) == im1_image.size(2) - 1) {
              return;
            }

            // Check if within masks
            typename Im1MaskType::value_type im1_mask_value = 1.0;
            if (im1_mask.valid()) {
              assign_pos_of (im1_image, 0, 3).to (im1_mask);
              im1_mask_value = im1_mask.value();
              if (im1_mask_value < 0.1) {
                im1_update.row(3).setZero();
                im2_update.row(3).setZero();
                return;
              }
            }

            typename Im2MaskType::value_type im2_mask_value = 1.0;
            if (im2_mask.valid()) {
              assign_pos_of (im2_image, 0, 3).to (im2_mask);
              im2_mask_value = im2_mask.value();
              if (im2_mask_value < 0.1) {
                im1_update.row(3).setZero();
                im2_update.row(3).setZero();
                return;
              }
            }


            // Compute cost
            default_type im1_speed = im2_image.value() - im1_image.value();
            default_type im2_speed = -im1_speed;
            if (std::abs (im1_speed) < robustness_parameter) {
              im1_speed = 0.0;
              im2_speed = 0.0;
            }


            default_type speed_squared = im1_speed * im1_speed;
            thread_cost += speed_squared;
            thread_voxel_count++;

            // Compute image 1 update
            assign_pos_of (im1_image, 0, 3).to (im2_gradient);
            Eigen::Matrix<typename Im1ImageType::value_type, 3, 1> im2_grad = im2_gradient.value();
            default_type denominator = speed_squared / normaliser + im2_grad.squaredNorm();
            if (std::abs (im1_speed) < intensity_difference_threshold || denominator < denominator_threshold)
              im1_update.row(3).setZero();
            else
              im1_update.row(3) = im1_speed * im2_grad.array() / denominator;

            // Compute image 2 update
            assign_pos_of (im2_image, 0, 3).to (im1_gradient);
            Eigen::Matrix<typename Im1ImageType::value_type, 3, 1> im1_grad = im1_gradient.value();
            denominator = speed_squared / normaliser + im1_grad.squaredNorm();
            if (std::abs (im2_speed) < intensity_difference_threshold || denominator < denominator_threshold)
              im2_update.row(3).setZero();
            else
              im2_update.row(3) = im2_speed * im1_grad.array() / denominator;
          }


          protected:
            default_type& global_cost;
            size_t& global_voxel_count;
            default_type thread_cost;
            size_t thread_voxel_count;
            default_type normaliser;
            const default_type robustness_parameter;
            const default_type intensity_difference_threshold;
            const default_type denominator_threshold;

            Adapter::Gradient3D<Im1ImageType> im1_gradient;
            Adapter::Gradient3D<Im2ImageType> im2_gradient; // TODO copy pointer?
            Im1MaskType im1_mask;
            Im2MaskType im2_mask;

      };
    }
  }
}
#endif
