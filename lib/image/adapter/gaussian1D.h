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
                      float stdev_in = 1.0,
                      size_t axis_in = 0,
                      size_t extent = 0) :
            Voxel<VoxelType> (parent),
            stdev (stdev_in),
            axis (axis_in) {
            if (!extent)
              radius_ = ceil(2.5 * stdev / vox(axis));
            else if (extent == 1)
              radius_ = 0;
            else
              radius_ = (extent - 1) / 2;
            compute_kernel();
          }

          typedef typename VoxelType::value_type value_type;

          value_type& value ()
          {
            if (!kernel.size()) {
              result = parent_vox.value();
              return result;
            }

            const ssize_t pos = (*this)[axis];
            const int from = pos < radius_ ? 0 : pos - radius_;
            const int to = pos >= (dim(axis) - radius_) ? dim(axis) - 1 : pos + radius_;

            result = 0.0;

            if (pos < radius_) {
              size_t c = radius_ - pos;
              value_type av_weights = 0.0;
              for (ssize_t k = from; k <= to; ++k) {
                av_weights += kernel[c];
                (*this)[axis] = k;
                result += value_type (parent_vox.value()) * kernel[c++];
              }
              result /= av_weights;
            } else if ((to - pos) < radius_){
              size_t c = 0;
              value_type av_weights = 0.0;
              for (ssize_t k = from; k <= to; ++k) {
                av_weights += kernel[c];
                (*this)[axis] = k;
                result += value_type (parent_vox.value()) * kernel[c++];
              }
              result /= av_weights;
            } else {
              size_t c = 0;
              for (ssize_t k = from; k <= to; ++k) {
                (*this)[axis] = k;
                result += value_type (parent_vox.value()) * kernel[c++];
              }
            }

            (*this)[axis] = pos;
            return result;
          }

          using Voxel<VoxelType>::name;
          using Voxel<VoxelType>::dim;
          using Voxel<VoxelType>::vox;
          using Voxel<VoxelType>::operator[];

        protected:

          void compute_kernel()
          {
            if ((radius_ < 1) || stdev <= 0.0)
              return;
            kernel.resize(2 * radius_ + 1);
            float norm_factor = 0.0;
            for (size_t c = 0; c < kernel.size(); ++c) {
              kernel[c] = exp(-((c-radius_) * (c-radius_) * vox(axis) * vox(axis))  / (2 * stdev * stdev));
              norm_factor += kernel[c];
            }
            for (size_t c = 0; c < kernel.size(); c++) {
              kernel[c] /= norm_factor;
            }
          }

          using Voxel<VoxelType>::parent_vox;
          float stdev;
          ssize_t radius_;
          size_t axis;
          std::vector<float> kernel;
          value_type result;
        };
    }
  }
}


#endif

