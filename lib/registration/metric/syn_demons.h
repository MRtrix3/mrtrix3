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

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      template <class InterpolatorType>
      class SynDemons {
        public:
          SynDemons (InterpolatorType& im1_image,
                     InterpolatorType& im2_image,
                     InterpolatorType& im1_mask,
                     InterpolatorType& im2_mask) :
                       im1_image (im1_image),
                       im2_image (im2_image),
                       im1_mask (im1_mask),
                       im2_mask (im2_mask){ }

          void ~SynDemons () {
            global_energy += thread_energy;
          }

          void operator() (Image<float>& im1_deform, Image<float>& im2_deform, Image<float>& im1_update, Image<float>& im2_update) {

            //Loop over non-boundary voxels
              //if (in both masks)
                //computeUpdate (weighted by mask)
                //computeUpdateInv (weighted by mask)

            if (im1_mask.valid()) {

            }

          }

        protected:
          InterpolatorType im1_image;
          InterpolatorType im2_image;
          InterpolatorType im1_mask;
          InterpolatorType im2_mask;

          default_type& global_energy;
          default_type thread_energy;

      };
    }
  }
}
#endif
