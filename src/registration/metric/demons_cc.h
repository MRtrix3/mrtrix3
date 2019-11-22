/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __registration_metric_demons_cc_h__
#define __registration_metric_demons_cc_h__


#include <mutex>

#include "image_helpers.h"
#include "adapter/gradient3D.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {

      template <class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
      class DemonsCC { MEMALIGN(DemonsCC<Im1ImageType,Im2ImageType,Im1MaskType,Im2MaskType>)
        public:
          DemonsCC (default_type& global_energy, size_t& global_voxel_count,
                     const Im1ImageType& im1_meansubtracted, const Im2ImageType& im2_meansubtracted, const Im1MaskType im1_mask, const Im2MaskType im2_mask) :
                       global_cost (global_energy),
                       global_voxel_count (global_voxel_count),
                       thread_cost (0.0),
                       thread_voxel_count (0),
                       mutex (new std::mutex),
                       normaliser (0.0),
                       robustness_parameter (1.e-12),
                       intensity_difference_threshold (0.001),
                       denominator_threshold (1e-9),
                       im1_gradient (im1_meansubtracted, true), im2_gradient (im2_meansubtracted, true),
                       im1_mask (im1_mask), im2_mask (im2_mask)
          {
            for (size_t d = 0; d < 3; ++d)
              normaliser += im1_meansubtracted.spacing(d) * im2_meansubtracted.spacing(d);
            normaliser /= 3.0;
          }

          ~DemonsCC () {
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

          void operator() (const Im1ImageType& im1_meansubtracted,
                           const Im2ImageType& im2_meansubtracted,
                           const Im2ImageType& A,
                           const Im2ImageType& B,
                           const Im2ImageType& C,
                           Image<default_type>& im1_update,
                           Image<default_type>& im2_update) {

            if (im1_meansubtracted.index(0) == 0 || im1_meansubtracted.index(0) == im1_meansubtracted.size(0) - 1 ||
                im1_meansubtracted.index(1) == 0 || im1_meansubtracted.index(1) == im1_meansubtracted.size(1) - 1 ||
                im1_meansubtracted.index(2) == 0 || im1_meansubtracted.index(2) == im1_meansubtracted.size(2) - 1) {
              im1_update.row(3) = 0.0;
              im2_update.row(3) = 0.0;
              return;
            }

            typename Im1MaskType::value_type im1_mask_value = 1.0;
            if (im1_mask.valid()) {
              assign_pos_of (im1_meansubtracted, 0, 3).to (im1_mask);
              im1_mask_value = im1_mask.value();
              if (im1_mask_value < 0.1) {
                im1_update.row(3) = 0.0;
                im2_update.row(3) = 0.0;
                return;
              }
            }

            typename Im2MaskType::value_type im2_mask_value = 1.0;
            if (im2_mask.valid()) {
              assign_pos_of (im2_meansubtracted, 0, 3).to (im2_mask);
              im2_mask_value = im2_mask.value();
              if (im2_mask_value < 0.1) {
                im1_update.row(3) = 0.0;
                im2_update.row(3) = 0.0;
                return;
              }
            }

            default_type sfm = A.value();
            default_type smm = B.value();
            default_type sff = C.value();
            default_type asq = sfm * sfm;

            default_type denom = smm * sff;
            if (std::isnan(sfm) || std::isnan(denom) || std::abs (denom) < denominator_threshold) {
              // if (std::abs (asq) > robustness_parameter)
              //   thread_voxel_count++; // TODO to count or not to count?
              im1_update.row(3) = 0.0;
              im2_update.row(3) = 0.0;
              return;
            }

            default_type lcc = asq / denom;
            thread_cost -= lcc;
            thread_voxel_count++;

            assign_pos_of (im1_meansubtracted, 0, 3).to (im1_gradient, im2_gradient);

            default_type i1 = im1_meansubtracted.value();
            default_type i2 = im2_meansubtracted.value();

            // Avants eq. 6 and 7
            Eigen::Matrix<typename Im1ImageType::value_type, 3, 1> grad =  2.0 * sfm / (sff * smm) * (
              (i2 - sfm / smm * i1 ) * im1_gradient.value() - (i1 - sfm / sff * i2 ) * im2_gradient.value());
            // TODO: add det(jacobian(Phi)))

            im1_update.row(3) = grad * 40.0; // TODO: normalise the update?
            im2_update.row(3) = -grad * 40.0;
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

      };
    }
  }
}
#endif
