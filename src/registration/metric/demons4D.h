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


#ifndef __registration_metric_demons4D_h__
#define __registration_metric_demons4D_h__

#include <mutex>

#include "image_helpers.h"
#include "adapter/gradient3D.h"
#include "registration/multi_contrast.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {

      template <class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
      class Demons4D { MEMALIGN(Demons4D<Im1ImageType,Im2ImageType,Im1MaskType,Im2MaskType>)
        public:
          Demons4D (default_type& global_energy, size_t& global_voxel_count,
                     const Im1ImageType& im1_image, const Im2ImageType& im2_image, const Im1MaskType im1_mask, const Im2MaskType im2_mask,
                     const vector<MultiContrastSetting>* contrast_settings = nullptr) :
                       global_cost (global_energy),
                       global_voxel_count (global_voxel_count),
                       thread_cost (0.0),
                       thread_voxel_count (0),
                       mutex (new std::mutex),
                       normaliser (0.0),
                       robustness_parameter (1.e12), // MP: used to be -1.e12 but then compared to std::abs(speed)
                       intensity_difference_threshold (0.001),
                       denominator_threshold (1e-9),
                       im1_gradient (im1_image, true), im2_gradient (im2_image, true),
                       im1_mask (im1_mask), im2_mask (im2_mask),
                       contrast_settings (contrast_settings),
                       nvols (im1_image.size(3))
          {
            for (size_t d = 0; d < 3; ++d)
              normaliser += im1_image.spacing(d) * im2_image.spacing(d);
            normaliser /= 3.0;

            weight.resize (nvols);
            speed.resize (nvols);
            speed_squared.resize (nvols);

            if (contrast_settings and contrast_settings->size() > 1) {
              for (const auto & mc : *contrast_settings)
                weight.segment(mc.start,mc.nvols).fill(mc.weight);
            } else
              weight.fill(1.0);
            DEBUG ("Demons4D weights: " + str(weight.transpose()));
          }

          ~Demons4D () {
            std::lock_guard<std::mutex> lock (*mutex);
            global_cost += thread_cost;
            global_voxel_count += thread_voxel_count;
          }

          void set_im1_mask (const Image<float>& mask) {
            im1_mask = mask;
          }

          void set_im2_mask (const Image<float>& mask) {
            im2_mask = mask;
          }


          void operator() (Im1ImageType& im1_image,
                           Im2ImageType& im2_image,
                           Image<default_type>& im1_update,
                           Image<default_type>& im2_update) {
            assert (im1_image.size(3) == nvols);
            assert (im2_image.size(3) == nvols);

            if (im1_image.index(0) == 0 || im1_image.index(0) == im1_image.size(0) - 1 ||
                im1_image.index(1) == 0 || im1_image.index(1) == im1_image.size(1) - 1 ||
                im1_image.index(2) == 0 || im1_image.index(2) == im1_image.size(2) - 1) {
              im1_update.row(3) = 0.0;
              im2_update.row(3) = 0.0;
              return;
            }


            typename Im1MaskType::value_type im1_mask_value = 1.0;
            if (im1_mask.valid()) {
              assign_pos_of (im1_image, 0, 3).to (im1_mask);
              im1_mask_value = im1_mask.value();
              if (im1_mask_value < 0.1) {
                im1_update.row(3) = 0.0;
                im2_update.row(3) = 0.0;
                return;
              }
            }

            typename Im2MaskType::value_type im2_mask_value = 1.0;
            if (im2_mask.valid()) {
              assign_pos_of (im2_image, 0, 3).to (im2_mask);
              im2_mask_value = im2_mask.value();
              if (im2_mask_value < 0.1) {
                im1_update.row(3) = 0.0;
                im2_update.row(3) = 0.0;
                return;
              }
            }

            assign_pos_of (im1_image, 0, 3).to (im1_gradient, im2_gradient);

            speed = im2_image.row(3);
            speed -= im1_image.row(3);
            // set speed to zero if abs(speed) < robustness_parameter
            speed = (speed.array().abs() < robustness_parameter).select(0.0, speed);
            speed_squared = speed.cwiseProduct(speed);

            thread_cost += weight.dot(speed_squared);
            thread_voxel_count += nvols;

            total_update.fill(0.0);
            for (ssize_t vol = 0; vol <= nvols; ++vol) {
              if (std::abs (speed[vol]) * weight[vol] < intensity_difference_threshold)
                continue;
              im1_gradient.index(3) = vol;
              im2_gradient.index(3) = vol;
              grad = (im2_gradient.value() + im1_gradient.value()).array() / 2.0;

              default_type denominator = speed_squared[vol] / normaliser + grad.squaredNorm();
              if (denominator < denominator_threshold)
                continue;
              total_update += (weight[vol] * speed[vol] / denominator) * grad;
            }
            total_update /= nvols;

            im1_update.row(3) = total_update;
            im2_update.row(3) = -total_update;
          }


          protected:
            default_type& global_cost;
            size_t& global_voxel_count;
            default_type thread_cost;
            size_t thread_voxel_count;
            std::shared_ptr<std::mutex> mutex;
            default_type normaliser;
            const default_type robustness_parameter;
            const default_type intensity_difference_threshold;
            const default_type denominator_threshold;

            Adapter::Gradient3D<Im1ImageType> im1_gradient;
            Adapter::Gradient3D<Im2ImageType> im2_gradient;
            Im1MaskType im1_mask;
            Im2MaskType im2_mask;

            const vector<MultiContrastSetting>* contrast_settings;
            const ssize_t nvols;
            Eigen::VectorXd weight;

            Eigen::Matrix<default_type, Eigen::Dynamic, 1> speed, speed_squared;
            Eigen::Vector3d total_update;
            Eigen::Matrix<default_type, 3, 1> grad;

      };
    }
  }
}
#endif
