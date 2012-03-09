/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 02/11/09.

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

#ifndef __image_adapter_extract_h__
#define __image_adapter_extract_h__

#include "image/adapter/voxel.h"
#include "image/position.h"

namespace MR
{
  namespace Image
  {
    namespace Adapter
    {

      template <class VoxelType> class Extract : public Voxel<VoxelType>
      {
        public:
          using Voxel<VoxelType>::ndim;
          using Voxel<VoxelType>::vox;
          using typename Voxel<VoxelType>::parent_vox;
          typedef typename VoxelType::value_type value_type;

          Extract (const VoxelType& original, const std::vector<std::vector<int> >& positions) :
            Voxel<VoxelType> (original),
            pos_ (ndim()), 
            indices_ (positions),
            transform_ (original.transform()) {
              reset();

              Math::Vector<float> a (4), b (4);
              a[0] = indices_[0][0] * vox (0);
              a[1] = indices_[1][0] * vox (1);
              a[2] = indices_[2][0] * vox (2);
              a[3] = 1.0;
              Math::mult (b, transform_, a);
              transform_.column (3) = b;
            }

          int dim (size_t axis) const {
            return indices_[axis].size();
          }

          const Math::Matrix<float>& transform () const {
            return transform_;
          }

          Image::Info info () const { 
            return Image::Info (*this);
          };

          void reset () {
            for (size_t n = 0; n < ndim(); ++n) {
              pos_[n] = 0;
              parent_vox[n] = indices_[n][0];
            }
          }

          Position<Extract<VoxelType> > operator[] (size_t axis) {
            return Position<Extract<VoxelType> > (*this, axis);
          }

        private:
          std::vector<size_t> pos_;
          const std::vector<std::vector<int> > indices_;
          Math::Matrix<float> transform_;

          ssize_t get_pos (size_t axis) const {
            return pos_[axis];
          }
          void set_pos (size_t axis, ssize_t position) {
            pos_[axis] = position;
            parent_vox[axis] = indices_[axis][position];
          }

          void move_pos (size_t axis, ssize_t increment) {
            int prev = pos_[axis];
            pos_[axis] += increment;
            parent_vox[axis] += indices_[axis][pos_[axis]] - indices_[axis][prev];
          }

          friend class Position<Extract<VoxelType> >;
      };

    }
  }
}

#endif


