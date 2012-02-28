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

#ifndef __mean_squared_metric_h__
#define __mean_squared_metric_h__

namespace MR
{
  namespace Registration
  {
    namespace Metric {
      class Base {

      public:
        MeanSquaredMetric () {}


        template <class ImageVoxelType, class MaskVoxelType, class TransformType, class InterpolatorType>
            class RunTime  {
          public:
            RunTime (const MeanSquaredMetric& parent, double& overall_cost_function) :
              ref_overall_cost_function (overall_cost_function),
              ref_overall_gradient (overall_gradient)
            {

            }
            ~RunTime () {
              ref_overall_cost_function += cost_function;
              ref_overall_gradient += gradient;
            }

            float operator() (Math::Vector<float>& params, Math::Vector<float>& gradient) {
               overall_cost_function = 0.0;
               overall_gradient.zero();

               ThreadedLoop ();

               gradient = overall_gradient;
               return overall_cost_function;
            }

            void operator() (const Iterator& voxel)
            {
              voxel_assign (*fixed);

              cost_function += jdflks;
            }

            ImageVoxelType fixed_image_;
            ImageVoxelType moving_image_;
            InterpolatorType interpolator_;
            TransformType transform_;
            Ptr<MaskVoxelType> fixed_mask_;
            Ptr<MaskVoxelType> moving_mask_;
            double cost_function, overall_cost_function;
            Math::Vector<float> gradient;

            double& ref_overall_cost_function;
            Math::Vector<float> ref_overall_gradient;
        };



    };
  }

  }
}

#endif
