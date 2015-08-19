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

#ifndef __registration_metric_base_h__
#define __registration_metric_base_h__

#include "filter/gradient.h"
#include "interp/linear.h"
#include "image.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      class Base {
        public:

          Base () :
            gradient_interp (nullptr),
            moving_grad (3) { }

          template <class MovingImageType>
          void set_moving_image (const MovingImageType& moving) {
            INFO ("Computing moving gradient...");
            MovingImageType moving_copy (moving);
            Filter::Gradient gradient_filter (moving_copy);
            gradient_ptr = std::make_shared<Image<float> >(Image<float>::scratch (gradient_filter));
            gradient_filter (moving_copy, *gradient_ptr);
            gradient_interp.reset (new Interp::Linear<Image<float> > (*gradient_ptr));
          }

        protected:

          // TODO replace this with in the fly gradient calculation
          void compute_moving_gradient (const Eigen::Vector3 moving_point) {
            gradient_interp->scanner (moving_point);
            moving_grad = gradient_interp->row(3).template cast<default_type>();
          }


          std::shared_ptr<Image<float> > gradient_ptr;
          MR::copy_ptr<Interp::Linear<Image<float> > > gradient_interp;
          Eigen::MatrixXd jacobian;
          Eigen::Vector3 moving_grad;
      };
    }
  }
}
#endif
