/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __image_registration_symmetric_adapter_crossCorrelation_h__
#define __image_registration_symmetric_adapter_crossCorrelation_h__

#include "math/median.h"
#include "image/adapter/voxel.h"

namespace MR
{
  namespace Image
  {

    namespace Adapter 
    {


      template <class VoxelType> 
        class CrossCorrelation3D : public Voxel<VoxelType> {
        public:
          CrossCorrelation3D (const VoxelType& parent) : 
            Voxel<VoxelType> (parent) {
              set_extent (std::vector<int>(1,9));
            }

          CrossCorrelation3D (const VoxelType& parent, const std::vector<int>& extent) : 
            Voxel<VoxelType> (parent) {
              set_extent (extent);
            }

          typedef typename VoxelType::value_type value_type;
          typedef CrossCorrelation3D voxel_type;

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

            WARN ("CrossCorrelation3D (just a copy of median3D) adapter for image \"" + name() + "\" initialised with extent " + str(extent_));

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

