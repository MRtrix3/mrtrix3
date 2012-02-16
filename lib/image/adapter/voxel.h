/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 23/05/09.

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

#ifndef __image_adapter_voxel_h__
#define __image_adapter_voxel_h__

#include "image/value.h"
#include "image/position.h"

namespace MR
{
  namespace Image
  {
    class Info;
    class Header;

    namespace Adapter
    {

      template <class VoxelType> 
        class Voxel {
          public:
            Voxel (const VoxelType& parent) :
              vox_ (parent) {
              }

            typedef typename VoxelType::value_type value_type;

            template <class U> 
              const Voxel& operator= (const U& V) {
                return vox_ = V;
              }

            const Image::Info& info () const {
              return vox_.info();
            }

            DataType datatype () const {
              return vox_.datatype();
            }
            const Math::Matrix<float>& transform () const {
              return vox_.transform();
            }

            ssize_t stride (size_t axis) const {
              return vox_ [axis];
            }
            size_t  ndim () const {
              return vox_.ndim();
            }
            ssize_t dim (size_t axis) const {
              return vox_.dim (axis);
            }
            float   vox (size_t axis) const {
              return vox_.vox (axis);
            }
            const std::string& name () const {
              return vox_.name();
            }


            ssize_t operator[] (size_t axis) const {
              return get_pos (axis);
            }
            Image::Position<Voxel> operator[] (size_t axis) {
              return Image::Position<Voxel> (*this, axis);
            }

            value_type value () const {
              return get_value ();
            }
            Image::Value<Voxel> value () {
              return Image::Value<Voxel> (*this);
            }


            friend std::ostream& operator<< (std::ostream& stream, const Voxel& V) {
              stream << "voxel for image \"" << V.name() << "\", position [ ";
              for (size_t n = 0; n < V.ndim(); ++n) 
                stream << V[n] << " ";
              stream << "], current offset = " << V.offset_ << ", value = " << V.value();
              return stream;
            }

          protected:
            VoxelType vox_;

            value_type get_value () const {
              return vox_.value();
            }

            void set_value (value_type val) {
              vox_.value() = val;
            }

            ssize_t get_pos (size_t axis) const {
              return vox_[axis];
            }
            void set_pos (size_t axis, ssize_t position) {
              vox_[axis] = position;
            }
            void move_pos (size_t axis, ssize_t increment) {
              vox_[axis] += increment;
            }

            friend class Image::Position<Voxel>;
            friend class Image::Value<Voxel>;
        };


    }
  }
}

#endif





