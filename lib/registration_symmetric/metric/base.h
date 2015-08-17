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

#ifndef __image_registration_symmetric_metric_base_h__
#define __image_registration_symmetric_metric_base_h__

#include "image/filter/gradient.h"
#include "image/interp/linear.h"
#include "image/voxel.h"
#include "image/buffer_scratch.h"

namespace MR
{
  namespace Image
  {
    namespace RegistrationSymmetric
    {
      namespace Metric
      {
        class Base {
          public:

            typedef double value_type;

            Base () :
              moving_gradient_interp (nullptr),
              template_gradient_interp (nullptr),
              moving_grad (3),
              template_grad (3) { }

            template <class MovingVoxelType>
            void set_moving_image (const MovingVoxelType& moving_voxel) {
              INFO ("Computing moving gradient...");
              MovingVoxelType moving_voxel_copy (moving_voxel);
              Image::Filter::Gradient gradient_filter (moving_voxel_copy);
              moving_gradient_data.reset (new Image::BufferScratch<float> (gradient_filter.info()));
              Image::BufferScratch<float>::voxel_type gradient_voxel_moving (*moving_gradient_data);
              gradient_filter (moving_voxel_copy, gradient_voxel_moving);
              moving_gradient_interp.reset (new Image::Interp::Linear<Image::BufferScratch<float>::voxel_type > (gradient_voxel_moving));
            }

            template <class TemplateVoxelType>
            void set_template_image (const TemplateVoxelType& template_voxel) {
              INFO ("Computing template gradient...");
              TemplateVoxelType template_voxel_copy (template_voxel);
              Image::Filter::Gradient gradient_filter (template_voxel_copy);
              template_gradient_data.reset (new Image::BufferScratch<float> (gradient_filter.info()));
              Image::BufferScratch<float>::voxel_type gradient_voxel_template (*template_gradient_data);
              gradient_filter (template_voxel_copy, gradient_voxel_template);
              template_gradient_interp.reset (new Image::Interp::Linear<Image::BufferScratch<float>::voxel_type > (gradient_voxel_template));
            }

          protected:

            void compute_moving_gradient (const Point<value_type> moving_point) {
              moving_gradient_interp->scanner (moving_point);
              (*moving_gradient_interp)[3] = 0;
              moving_grad[0] = moving_gradient_interp->value();
              ++(*moving_gradient_interp)[3];
              moving_grad[1] = moving_gradient_interp->value();
              ++(*moving_gradient_interp)[3];
              moving_grad[2] = moving_gradient_interp->value();
            }

            void compute_template_gradient (const Point<value_type> template_point) {
              template_gradient_interp->scanner (template_point);
              (*template_gradient_interp)[3] = 0;
              template_grad[0] = template_gradient_interp->value();
              ++(*template_gradient_interp)[3];
              template_grad[1] = template_gradient_interp->value();
              ++(*template_gradient_interp)[3];
              template_grad[2] = template_gradient_interp->value();
            }


            std::shared_ptr<Image::BufferScratch<float> > moving_gradient_data;
            std::shared_ptr<Image::BufferScratch<float> > template_gradient_data;
            MR::copy_ptr<Image::Interp::Linear<Image::BufferScratch<float>::voxel_type> > moving_gradient_interp;
            MR::copy_ptr<Image::Interp::Linear<Image::BufferScratch<float>::voxel_type> > template_gradient_interp;
            Math::Matrix<double> jacobian;
            Math::Vector<double> moving_grad;
            Math::Vector<double> template_grad;
        };
      }
    }
  }
}
#endif
