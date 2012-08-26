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

#ifndef __registration_mean_squared_metric_h__
#define __registration_mean_squared_metric_h__

#include "image/filter/gradient3D.h"
#include "image/filter/gaussian_smooth.h"
#include "image/interp/linear.h"
#include "image/header.h"
#include "image/voxel.h"
#include "image/buffer_scratch.h"
#include "point.h"
#include "math/vector.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      class MeanSquared {
        public:
          MeanSquared () :
            gradient_interp_(NULL) { }


          template <class Params>
            double operator() (Params& params,
                              Point<double> target_point,
                              Point<double> moving_point,
                              Math::Vector<double>& gradient) {

              params.transformation.get_jacobian_wrt_params (target_point, jacobian_);
              Math::Vector<double> moving_grad (3);

              gradient_interp_->scanner(moving_point);
              (*gradient_interp_)[3] = 0;
              moving_grad[0] = gradient_interp_->value();
              ++(*gradient_interp_)[3];
              moving_grad[1] = gradient_interp_->value();
              ++(*gradient_interp_)[3];
              moving_grad[2] = gradient_interp_->value();

              double diff = params.moving_image_interp.value() - params.target_image.value();

              for (size_t par = 0; par < gradient.size(); par++) {
                double sum = 0.0;
                for( size_t dim = 0; dim < 3; dim++) {
                  sum += 2.0 * diff * jacobian_(dim, par) * moving_grad[dim];
                }
                gradient[par] += sum;
              }
              return diff * diff;
          }

          template <class MovingVoxelType>
          void set_moving_image (const MovingVoxelType& moving_image) {

            INFO ("Computing moving gradient...");

            typedef typename MovingVoxelType::voxel_type MovingVoxelType;
            MovingVoxelType moving_voxel (moving_image);

            Image::Filter::GaussianSmooth smooth_filter (moving_voxel);
            Image::BufferScratch<float> smoothed_data (smooth_filter.info());
            Image::BufferScratch<float>::voxel_type smoothed_voxel (smoothed_data);
            {
              LogLevelLatch loglevel (0);
              smooth_filter (moving_voxel, smoothed_voxel);
            }

            Image::Filter::Gradient3D gradient_filter (smoothed_voxel);
            gradient_data_= new Image::BufferScratch<float> (gradient_filter.info());
            Image::BufferScratch<float>::voxel_type gradient_voxel (*gradient_data_);
            gradient_filter (smoothed_voxel, gradient_voxel);

            gradient_interp_ = new Image::Interp::Linear<Image::BufferScratch<float>::voxel_type > (gradient_voxel);
          }

        protected:
          RefPtr<Image::BufferScratch<float> > gradient_data_;
          Ptr<Image::Interp::Linear<Image::BufferScratch<float>::voxel_type> > gradient_interp_;
          Math::Matrix<double> jacobian_;
      };
    }
  }
}
#endif
