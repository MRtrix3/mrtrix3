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

#ifndef __image_registration_metric_base_h__
#define __image_registration_metric_base_h__

#include "image/filter/gradient.h"
#include "image/interp/linear.h"
#include "image/voxel.h"
#include "image/buffer_scratch.h"

namespace MR
{
  namespace Image
  {
    namespace Registration
    {
      namespace Metric
      {
        class Base {
          public:

            typedef double value_type;

            Base () :
              gradient_interp (nullptr),
              moving_grad (3) { }

            template <class MovingVoxelType>
            void set_moving_image (const MovingVoxelType& moving_voxel) {
              INFO ("Computing moving gradient...");
              MovingVoxelType moving_voxel_copy (moving_voxel);
              Image::Filter::Gradient gradient_filter (moving_voxel_copy);
              gradient_data.reset (new Image::BufferScratch<float> (gradient_filter.info()));
              Image::BufferScratch<float>::voxel_type gradient_voxel (*gradient_data);
              gradient_filter (moving_voxel_copy, gradient_voxel);
              gradient_interp.reset (new Image::Interp::Linear<Image::BufferScratch<float>::voxel_type > (gradient_voxel));
            }

          protected:

            void compute_moving_gradient (const Point<value_type> moving_point) {
              gradient_interp->scanner (moving_point);
              (*gradient_interp)[3] = 0;
              moving_grad[0] = gradient_interp->value();
              ++(*gradient_interp)[3];
              moving_grad[1] = gradient_interp->value();
              ++(*gradient_interp)[3];
              moving_grad[2] = gradient_interp->value();
            }


            std::shared_ptr<Image::BufferScratch<float> > gradient_data;
            MR::copy_ptr<Image::Interp::Linear<Image::BufferScratch<float>::voxel_type> > gradient_interp;
            Math::Matrix<double> jacobian;
            Math::Vector<double> moving_grad;
        };
      }
    }
  }
}
#endif
