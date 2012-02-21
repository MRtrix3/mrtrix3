/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 17/02/12.

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

#ifndef __image_adapter_gaussian1D_h__
#define __image_adapter_gaussian1D_h__

#include "image/adapter/voxel.h"

namespace MR
{
  namespace Image
  {
    namespace Adapter
    {

      template <class VoxelType>
        class Gaussian1D : public Voxel<VoxelType> {
        public:
          Gaussian1D (const VoxelType& parent,
                      float stdev = 1.0,
                      size_t axis = 0,
                      size_t extent = 0) :
            Voxel<VoxelType> (parent),
            stdev_(stdev),
            axis_(axis) {

            if (!extent) {
              extent_ = 2 * ceil((4 * stdev_) / vox(axis_)) - 1;
            } else {
              extent_ = extent;
            }
            compute_kernel();
          }

          typedef typename VoxelType::value_type value_type;

          value_type value () {
            if (kernel.size() == 1)
              return vox_.value();

            const ssize_t pos [3] = { (*this)[0], (*this)[1], (*this)[2] };
            const int radius = floor(static_cast<float>(extent_)/2.0);
            const int from = pos[axis_] < radius ? 0 : pos[axis_] - radius;
            const int to = pos[axis_] >= (dim(axis_) - radius) ? dim(axis_) - 1 : pos[axis_] + radius;

            value_type val = 0;

            if (pos[axis_] < radius) {
              size_t c = radius - pos[axis_];
              value_type av_weights = 0.0;
              for (ssize_t k = from; k <= to; ++k) {
                av_weights += kernel[c];
                (*this)[axis_] = k;
                val += vox_.value() * kernel[c++];
              }
              val /= av_weights;
            } else if ((to - pos[axis_]) < radius){
              size_t c = 0;
              value_type av_weights = 0.0;
              for (ssize_t k = from; k <= to; ++k) {
                av_weights += kernel[c];
                (*this)[axis_] = k;
                val += vox_.value() * kernel[c++];
              }
              val /= av_weights;
            } else {
              size_t c = 0;
              for (ssize_t k = from; k <= to; ++k) {
                (*this)[axis_] = k;
                val += vox_.value() * kernel[c++];
              }
            }

            (*this)[0] = pos[0];
            (*this)[1] = pos[1];
            (*this)[2] = pos[2];

            return val;
          }

          using Voxel<VoxelType>::name;
          using Voxel<VoxelType>::dim;
          using Voxel<VoxelType>::vox;
          using Voxel<VoxelType>::operator[];

        protected:

          void compute_kernel() {
            if ((extent_ <= 1) || stdev_ <= 0.0) {
              kernel.resize(1);
              return;
            }

            kernel.resize(extent_);
            float norm_factor = 0.0;
            for (size_t c = 0; c < extent_; c++) {
              kernel[c] = (1/(stdev_* sqrt(2*M_PI))) *
                  exp(-((c-floor(static_cast<float>(extent_) / 2.0)) *
                        (c-floor(static_cast<float>(extent_) / 2.0)) *
                         vox(axis_) * vox(axis_)) / (2 * stdev_ * stdev_));
              norm_factor += kernel[c];
            }
            for (size_t c = 0; c < extent_; c++) {
              kernel[c] /= norm_factor;
            }
          }

          using Voxel<VoxelType>::vox_;
          float stdev_;
          size_t extent_;
          size_t axis_;
          std::vector<value_type> kernel;
        };
    }
  }
}


#endif

