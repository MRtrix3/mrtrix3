/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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

