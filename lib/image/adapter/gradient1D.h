#ifndef __image_adapter_gradient1D_h__
#define __image_adapter_gradient1D_h__

#include "image/adapter/voxel.h"

namespace MR
{
  namespace Image
  {
    namespace Adapter
    {

      template <class VoxelType>
        class Gradient1D : public Voxel<VoxelType> {
        public:
          Gradient1D (const VoxelType& parent,
                      size_t axis = 0) :
            Voxel<VoxelType> (parent),
            axis_(axis) { }

          typedef typename VoxelType::value_type value_type;

          void set_axis(size_t axis) {
            axis_ = axis;
          }

          float& value () {
            const ssize_t pos = (*this)[axis_];
            result = 0.0;

            if (pos == 0) {
              result = parent_vox.value();
              (*this)[axis_] = pos + 1;
              result = parent_vox.value() - result;
            } else if (pos == dim(axis_) - 1) {
              result = parent_vox.value();
              (*this)[axis_] = pos - 1;
              result -= parent_vox.value();
            } else {
              (*this)[axis_] = pos + 1;
              result = parent_vox.value();
              (*this)[axis_] = pos - 1;
              result = 0.5 * (result - parent_vox.value());
            }
            (*this)[axis_] = pos;

            return result;
          }

          using Voxel<VoxelType>::name;
          using Voxel<VoxelType>::dim;
          using Voxel<VoxelType>::vox;
          using Voxel<VoxelType>::operator[];

        protected:
          using Voxel<VoxelType>::parent_vox;
          size_t axis_;
          value_type result;
        };
    }
  }
}


#endif

