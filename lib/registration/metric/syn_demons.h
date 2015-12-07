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

      template <class Im1ImageType, class Im2ImageType>
      class SynDemons {
        public:
          SynDemons (Im1ImageType& im1_image, ){}

          void ~SynDemons () {
            global_energy += thread_energy;
          }

          template <class Im1UpdateVecType, class Im2UpdateVecType>
            default_type operator() (const Im1ImageType& im1_image,
                                     const Im2ImageType& im2_image,
                                     Im1UpdateVecType& im1_update,
                                     Im2UpdateVecType& im2_update) {

              typename Im1ImageType::ValueType im1_value;
              typename Im2ImageType::ValueType im2_value;


            protected:
              Adapter::Gradient3D<Im1ImageType> im1_gradient;
              Adapter::Gradient3D<Im2ImageType> im2_gradient;

              // Need to adjust



          }
      };
    }
  }
}
#endif
