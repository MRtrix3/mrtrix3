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

      template <class VoxelType> class Extract1D : public Voxel<VoxelType>
      {
        public:
          using Voxel<VoxelType>::ndim;
          using Voxel<VoxelType>::vox;
          using typename Voxel<VoxelType>::parent_vox;
          typedef typename VoxelType::value_type value_type;

          Extract1D (const VoxelType& original, const size_t axis, const std::vector<int>& indices) :
              Voxel<VoxelType> (original),
              extract_axis (axis),
              indices (indices),
              trans (original.transform()) {
                reset();

                if (extract_axis < 3) {
                  Math::Vector<float> a (4), b (4);
                  a = 0.0;
                  a[extract_axis] = indices[0] * vox (extract_axis);
                  a[3] = 1.0;
                  Math::mult (b, trans, a);
                  trans.column (3) = b;
                }
              }

          int dim (size_t axis) const {
            return ( axis == extract_axis ? indices.size() : Voxel<VoxelType>::dim (axis) );
          }

          const Math::Matrix<float>& transform () const {
            return trans;
          }

          Image::Info info () const { 
            return Image::Info (*this);
          };

          void reset () {
            for (size_t n = 0; n < ndim(); ++n) 
              parent_vox[n] = ( n == extract_axis ? indices[0] : 0 );
            current_pos = 0;
          }

          Position<Extract1D<VoxelType> > operator[] (size_t axis) {
            return Position<Extract1D<VoxelType> > (*this, axis);
          }

          int operator[] (size_t axis) const {
            return get_pos (axis);
          }

          friend std::ostream& operator<< (std::ostream& stream, const Extract1D& V) {
            stream << "Extract1D voxel adapter for image \"" << V.name() << "\", datatype " << V.datatype().specifier() << ", position [ ";
            for (size_t n = 0; n < V.ndim(); ++n) 
              stream << V[n] << " ";
            stream << "], value = " << V.value();
            return stream;
          }

        private:
          size_t extract_axis;
          const std::vector<int> indices;
          Math::Matrix<float> trans;
          int current_pos;

          int get_pos (size_t axis) {
            return ( axis == extract_axis ? current_pos : parent_vox[axis] );
          }

          void set_pos (size_t axis, int position) {
            if (axis == extract_axis) {
              parent_vox[axis] = indices[position];
              current_pos = position;
            }
            else 
              parent_vox[axis] = position;
          }

          void move_pos (size_t axis, int increment) {
            if (axis == extract_axis) {
              int prev_pos = indices[current_pos];
              current_pos += increment;
              parent_vox[axis] += current_pos < int(indices.size()) ? indices[current_pos] - prev_pos : 0;
            }
            else 
              parent_vox[axis] += increment;
          }

          friend class Position<Extract1D<VoxelType> >;
      };










      template <class VoxelType> class Extract : public Voxel<VoxelType>
      {
        public:
          using Voxel<VoxelType>::ndim;
          using Voxel<VoxelType>::vox;
          using typename Voxel<VoxelType>::parent_vox;
          typedef typename VoxelType::value_type value_type;

          Extract (const VoxelType& original, const std::vector<std::vector<int> >& indices) :
            Voxel<VoxelType> (original),
            current_pos (ndim()), 
            indices (indices),
            trans (original.transform()) {
              reset();

              Math::Vector<float> a (4), b (4);
              a[0] = indices[0][0] * vox (0);
              a[1] = indices[1][0] * vox (1);
              a[2] = indices[2][0] * vox (2);
              a[3] = 1.0;
              Math::mult (b, trans, a);
              trans.column (3) = b;
            }

          int dim (size_t axis) const {
            return indices[axis].size();
          }

          const Math::Matrix<float>& transform () const {
            return trans;
          }

          Image::Info info () const { 
            return Image::Info (*this);
          };

          void reset () {
            for (size_t n = 0; n < ndim(); ++n) {
              current_pos[n] = 0;
              parent_vox[n] = indices[n][0];
            }
          }

          Position<Extract<VoxelType> > operator[] (size_t axis) {
            return Position<Extract<VoxelType> > (*this, axis);
          }

        private:
          std::vector<size_t> current_pos;
          const std::vector<std::vector<int> > indices;
          Math::Matrix<float> trans;

          int get_pos (size_t axis) const {
            return current_pos[axis];
          }
          void set_pos (size_t axis, int position) {
            current_pos[axis] = position;
            parent_vox[axis] = indices[axis][position];
          }

          void move_pos (size_t axis, int increment) {
            int prev = current_pos[axis];
            current_pos[axis] += increment;
            parent_vox[axis] += indices[axis][current_pos[axis]] - indices[axis][prev];
          }

          friend class Position<Extract<VoxelType> >;
      };

    }
  }
}

#endif


