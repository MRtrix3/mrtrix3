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
            if (!extent)
              radius_ = ceil(2.5 * stdev_ / vox(axis_));
            else if (extent == 1)
              radius_ = 0;
            else
              radius_ = (extent - 1) / 2;
            compute_kernel();
          }

          typedef typename VoxelType::value_type value_type;

          value_type value ()
          {
            if (!kernel_.size())
              return parent_vox.value();

            const ssize_t pos = (*this)[axis_];
            const int from = pos < radius_ ? 0 : pos - radius_;
            const int to = pos >= (dim(axis_) - radius_) ? dim(axis_) - 1 : pos + radius_;

            value_type val = 0;

            if (pos < radius_) {
              size_t c = radius_ - pos;
              value_type av_weights = 0.0;
              for (ssize_t k = from; k <= to; ++k) {
                av_weights += kernel_[c];
                (*this)[axis_] = k;
                val += parent_vox.value() * kernel_[c++];
              }
              val /= av_weights;
            } else if ((to - pos) < radius_){
              size_t c = 0;
              value_type av_weights = 0.0;
              for (ssize_t k = from; k <= to; ++k) {
                av_weights += kernel_[c];
                (*this)[axis_] = k;
                val += parent_vox.value() * kernel_[c++];
              }
              val /= av_weights;
            } else {
              size_t c = 0;
              for (ssize_t k = from; k <= to; ++k) {
                (*this)[axis_] = k;
                val += parent_vox.value() * kernel_[c++];
              }
            }

            (*this)[axis_] = pos;
            return val;
          }

          using Voxel<VoxelType>::name;
          using Voxel<VoxelType>::dim;
          using Voxel<VoxelType>::vox;
          using Voxel<VoxelType>::operator[];

        protected:

          void compute_kernel()
          {
            if ((radius_ < 1) || stdev_ <= 0.0)
              return;
            kernel_.resize(2 * radius_ + 1);
            float norm_factor = 0.0;
            for (size_t c = 0; c < kernel_.size(); ++c) {
              kernel_[c] = exp(-((c-radius_) * (c-radius_) * vox(axis_) * vox(axis_))  / (2 * stdev_ * stdev_));
              norm_factor += kernel_[c];
            }
            for (size_t c = 0; c < kernel_.size(); c++) {
              kernel_[c] /= norm_factor;
            }
          }

          using Voxel<VoxelType>::parent_vox;
          float stdev_;
          ssize_t radius_;
          size_t axis_;
          std::vector<value_type> kernel_;
        };
    }
  }
}


#endif

