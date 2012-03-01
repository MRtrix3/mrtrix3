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

#ifndef __registration_metric_evaluate_h__
#define __registration_metric_evaluate_h__

#include "registration/metric/thread_kernel.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      template <class MetricType, class ParamType>
        class Evaluate {
          public:

            typedef typename ParamType::TransformParamType TransformParamType;

            Evaluate (const MetricType& metric, ParamType& parameters) :
              metric_ (metric),
              params_ (parameters) { }


            float operator() (const Math::Vector<float>& x, Math::Vector<float>& gradient) {

              double overall_cost_function = 0.0;
              gradient.zero();
              params_.transformation.set_parameter_vector(x);
              std::cout << "Evaluate operator " <<  gradient  << std::endl;
              ThreadKernel<MetricType, ParamType> kernel (metric_, params_, overall_cost_function, gradient);

              Image::threaded_loop (kernel, params_.target_image, 2, 0, 3);
              std::cout << "Evaluate operator" << std::endl;
              return overall_cost_function;
            }

            size_t size() {
              return params_.transformation.get_parameter_vector().size();
            }

            float init (Math::Vector<TransformParamType>& x) {
              for (size_t i = 0; i < size(); i++)
                x[i] = params_.transformation.get_parameter_vector()[i];
              return 0.01; // return init step size. TODO confirm this is appropriate;
            }

          protected:
              MetricType metric_;
              ParamType params_;
      };
    }
  }
}

#endif
