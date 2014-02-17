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


#ifndef __image_adapter_median3D_h__
#define __image_adapter_median3D_h__

#include "math/median.h"
#include "image/adapter/voxel.h"

namespace MR
{
  namespace Image
  {
    namespace Adapter 
    {


      template <class VoxelType> 
        class Median3D : public Voxel<VoxelType> {
        public:
          Median3D (const VoxelType& parent) : 
            Voxel<VoxelType> (parent) {
              set_extent (std::vector<int>(1,3));
            }

          Median3D (const VoxelType& parent, const std::vector<int>& extent) : 
            Voxel<VoxelType> (parent) {
              set_extent (extent);
            }

          typedef typename VoxelType::value_type value_type;
          typedef Median3D voxel_type;

          void set_extent (const std::vector<int>& extent) {
            for (size_t i = 0; i < extent.size(); ++i)
              if (! (extent[i] & int(1)))
                throw Exception ("expected odd number for extent");
            if (extent.size() != 1 && extent.size() != 3)
              throw Exception ("unexpected number of elements specified in extent");
            if (extent.size() == 1)
              extent_ = std::vector<int> (3, extent[0]);
            else 
              extent_ = extent;

            DEBUG ("median3D adapter for image \"" + name() + "\" initialised with extent " + str(extent_));

            for (size_t i = 0; i < 3; ++i)
              extent_[i] = (extent_[i]-1)/2;
          }



          value_type& value () {

            const ssize_t old_pos [3] = { (*this)[0], (*this)[1], (*this)[2] };
            const ssize_t from[3] = {
              (*this)[0] < extent_[0] ? 0 : (*this)[0] - extent_[0], 
              (*this)[1] < extent_[1] ? 0 : (*this)[1] - extent_[1], 
              (*this)[2] < extent_[2] ? 0 : (*this)[2] - extent_[2]
            };
            const ssize_t to[3] = {
              (*this)[0] >= dim(0)-extent_[0] ? dim(0) : (*this)[0]+extent_[0]+1, 
              (*this)[1] >= dim(1)-extent_[1] ? dim(1) : (*this)[1]+extent_[1]+1, 
              (*this)[2] >= dim(2)-extent_[2] ? dim(2) : (*this)[2]+extent_[2]+1
            };

            values.clear();

            for ((*this)[2] = from[2]; (*this)[2] < to[2]; ++(*this)[2]) 
              for ((*this)[1] = from[1]; (*this)[1] < to[1]; ++(*this)[1]) 
                for ((*this)[0] = from[0]; (*this)[0] < to[0]; ++(*this)[0]) 
                  values.push_back (parent_vox.value());

            (*this)[0] = old_pos[0];
            (*this)[1] = old_pos[1];
            (*this)[2] = old_pos[2];

            retval = Math::median (values);
            return retval;
          }

          using Voxel<VoxelType>::name;
          using Voxel<VoxelType>::dim;
          using Voxel<VoxelType>::operator[];

        protected:
          using Voxel<VoxelType>::parent_vox;
          std::vector<int> extent_;
          std::vector<value_type> values;
          value_type retval;
        };

    }
  }
}


#endif

