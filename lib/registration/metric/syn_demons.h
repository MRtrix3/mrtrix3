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
      class SynDemons {
        public:
          SynDemons (default_type& global_energy,
                     const Im1ImageType& im1_image, const Im2ImageType& im2_image, const Im1MaskType im1_mask, const Im2MaskType im2_mask) :
                       global_energy (global_energy),
                       thread_energy (0.0),
                       im1_gradient (im1_image, true), im2_gradient (im2_image, true),
                       im1_mask (im1_mask), im2_mask (im2_mask) {}

          ~SynDemons () {
            global_energy += thread_energy;
          }

          void set_im1_mask (const Image<float>& mask) {
            im1_mask = mask;
          }

          void set_im2_mask (const Image<float>& mask) {
            im2_mask = mask;
          }

          template <class Im1UpdateImageType, class Im2UpdateImageType>
            default_type operator() (const Im1ImageType& im1_image,
                                     const Im2ImageType& im2_image,
                                     Im1UpdateImageType& im1_update,
                                     Im2UpdateImageType& im2_update) {

              float im1_mask_value = 1.0;
              if (im1_mask.valid()) {
                assign_pos_of (im1_image).to (im1_mask);
                im1_mask_value = im1_mask.value();
                if (im1_mask_value < 0.1) {
                  im1_update.row(3).setZero();
                  im2_update.row(3).setZero();
                  return;
                }
              }

              float im2_mask_value = 1.0;
              if (im2_mask.valid()) {
                assign_pos_of (im2_image).to (im2_mask);
                im2_mask_value = im2_mask.value();
                if (im2_mask_value < 0.1)
                  im1_update.row(3).setZero();
                  im2_update.row(3).setZero();
                  return;
              }

              assign_pos_of (im1_image, 0, 3).to (im1_gradient);
              assign_pos_of (im2_image, 0, 3).to (im2_gradient);


              typename Im1ImageType::ValueType im1_value;
              typename Im2ImageType::ValueType im2_value;

              Eigen::Matrix<float, 1, 3> im1_grad;
              Eigen::Matrix<float, 1, 3> im2_grad;
            }



            protected:
              default_type& global_energy;
              default_type thread_energy;
              Adapter::Gradient3D<Im1ImageType> im1_gradient;
              Adapter::Gradient3D<Im2ImageType> im2_gradient; // TODO copy pointer?
              Im1MaskType im1_mask;
              Im2MaskType im2_mask;

      };
    }
  }
}
#endif
