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

#ifndef __registration_linear_registration_h__
#define __registration_linear_registration_h__

#include "ptr.h"
#include "math/matrix.h"

namespace MR
{
  namespace Registration
  {
    class LinearRegistration
    {

      public:
        LinearRegistration () :
            max_iter_ (1, 200),
            nlevels_ (1) { }

        ~LinearRegistration () { }

        void set_max_iter (const std::vector<int>& max_iter) {
          max_iter_ = max_iter;
        }

        template<class ImageVoxelType, class MetricType, class InterpolatorType, class TransformationType>
        void run (ImageVoxelType& moving,
                  ImageVoxelType& target,
                  MetricType& metric) {


            MetricType::RunTime runtime();
            runtime.mask

            // for each level
            //   Compute downsampled image
            //   Compute downsampled mask
            //   Set MovingImage to Metric
            //   Set FixedImage to Metric
            //   Set Masks to Metric
            //   Initialise metric
            //
            //
            //
            //


        }

      private:
        std::vector<int> max_iter_;
        size_t nlevels_;

    };

  }
}

#endif
