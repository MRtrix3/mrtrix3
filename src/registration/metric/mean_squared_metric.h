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
#include "image/filter/gaussian3D.h"
#include "image/interp/nearest.h"
#include "image/header.h"
#include "image/voxel.h"
#include "image/buffer_scratch.h"

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
          float operator() (Params& params, Math::Vector<float>& gradient) {
              assert(gradient_interp_);
//              gradient += my_gradient_at_this_point();
              return params.target_image.value() - params.moving_image.value();

          }

          template <class MovingDataType>
          void set_moving_image (MovingDataType& moving_data) {

            typedef typename MovingDataType::voxel_type MovingVoxelType;
            MovingVoxelType moving_voxel(moving_data);

            Image::Filter::Gaussian3D smooth_filter (moving_voxel);
            Image::Filter::Gradient3D gradient_filter (moving_voxel);

            Image::Header smooth_header(moving_data);
            smooth_header.info() = smooth_filter.info();
            Image::BufferScratch<float> smoothed_data (smooth_header);
            Image::BufferScratch<float>::voxel_type smoothed_voxel (smoothed_data);

            Image::Header gradient_header(moving_data);
            gradient_header.info() = gradient_filter.info();
            Image::BufferScratch<float> gradient_data (gradient_header);
            Image::BufferScratch<float>::voxel_type gradient_voxel (gradient_data);

            smooth_filter (moving_voxel, smoothed_voxel);
            gradient_filter (smoothed_voxel, gradient_voxel);

            gradient_interp_ = new Image::Interp::Nearest<Image::BufferScratch<float>::voxel_type > (gradient_voxel);
          }

        protected:

          Ptr<Image::Interp::Nearest<Image::BufferScratch<float>::voxel_type> > gradient_interp_;
      };
    }
  }
}
#endif
