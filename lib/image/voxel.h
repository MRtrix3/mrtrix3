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

#ifndef __image_voxel_h__
#define __image_voxel_h__

#include "debug.h"
#include "get_set.h"
#include "image/header.h"
#include "image/stride.h"
#include "image/value.h"
#include "image/position.h"
#include "image/threaded_copy.h"
#include "image/buffer.h"

namespace MR
{
  namespace Image
  {

    template <class BufferType>
      class Voxel {
        public:
          Voxel (BufferType& array) :
            data_ (array),
            stride_ (Image::Stride::get_actual (data_)),
            start_ (Image::Stride::offset (*this)),
            offset_ (start_),
            x (ndim(), 0) {
              DEBUG ("voxel accessor for image \"" + name() + "\" initialised with start = " + str (start_) + ", strides = " + str (stride_));
            }

          typedef typename BufferType::value_type value_type;
          typedef Voxel voxel_type;

          const Info& info () const {
            return data_.info();
          }
          const BufferType& buffer () const {
            return data_;
          }

          DataType datatype () const {
            return data_.datatype();
          }
          const Math::Matrix<float>& transform () const {
            return data_.transform();
          }

          ssize_t stride (size_t axis) const {
            return stride_ [axis];
          }
          size_t  ndim () const {
            return data_.ndim();
          }
          ssize_t dim (size_t axis) const {
            return data_.dim (axis);
          }
          float   vox (size_t axis) const {
            return data_.vox (axis);
          }
          const std::string& name () const {
            return data_.name();
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

          //! return RAM address of current voxel
          /*! \note this will only work with Image::BufferPreload and
           * Image::BufferScratch. */
          value_type* address () const {
            return data_.address (offset_);
          }

          bool valid (size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) const {
            to_axis = std::min (to_axis, ndim());
            for (size_t n = from_axis; n < to_axis; ++n)
              if (get_pos(n) < 0 || get_pos(n) >= dim(n))
                return false;
            return true;
          }

          friend std::ostream& operator<< (std::ostream& stream, const Voxel& V) {
            stream << "voxel for image \"" << V.name() << "\", datatype " << V.datatype().specifier() << ", position [ ";
            for (size_t n = 0; n < V.ndim(); ++n)
              stream << V[n] << " ";
            stream << "], current offset = " << V.offset_ << ", value = " << V.value();
            return stream;
          }

          std::string save (const std::string& filename) const 
          {
            Voxel in (*this);
            Image::Header header;
            header.info() = info();
            Image::Buffer<value_type> buffer_out (filename, header);
            typename Image::Buffer<value_type>::voxel_type out (buffer_out);
            Image::threaded_copy (in, out);
            return buffer_out.__get_handler()->files[0].name;
          }


          void display () const {
            std::string filename = save ("-");
            CONSOLE ("displaying image " + filename);
            if (system (("bash -c \"mrview " + filename + "\"").c_str()))
              WARN (std::string("error invoking viewer: ") + strerror(errno));
          }

        protected:
          BufferType& data_;
          const std::vector<ssize_t> stride_;
          const size_t start_;
          size_t offset_;
          std::vector<ssize_t> x;

          value_type get_value () const {
            return data_.get_value (offset_);
          }

          void set_value (value_type val) {
            data_.set_value (offset_, val);
          }

          ssize_t get_pos (size_t axis) const {
            return x[axis];
          }
          void set_pos (size_t axis, ssize_t position) {
            offset_ += stride (axis) * (position - x[axis]);
            x[axis] = position;
          }
          void move_pos (size_t axis, ssize_t increment) {
            offset_ += stride (axis) * increment;
            x[axis] += increment;
          }

          friend class Image::Position<Voxel>;
          friend class Image::Value<Voxel>;
      };


    template <class InputVoxelType, class OutputVoxelType>
      void voxel_assign (OutputVoxelType& out, const InputVoxelType& in, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) {
        to_axis = std::min (to_axis, std::min (in.ndim(), out.ndim()));
        for (size_t n = from_axis; n < to_axis; ++n)
          out[n] = in[n];
      }

    template <class InputVoxelType, class OutputVoxelType, class OutputVoxelType2>
      void voxel_assign2 (OutputVoxelType& out, OutputVoxelType2& out2, const InputVoxelType& in, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) {
        to_axis = std::min (to_axis, std::min (in.ndim(), std::min (out.ndim(), out2.ndim())));
        for (size_t n = from_axis; n < to_axis; ++n)
          out[n] = out2[n] = in[n];
      }

    template <class InputVoxelType, class OutputVoxelType, class OutputVoxelType2, class OutputVoxelType3>
      void voxel_assign3 (OutputVoxelType& out, OutputVoxelType2& out2, OutputVoxelType2& out3, const InputVoxelType& in, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) {
        to_axis = std::min (to_axis, std::min (in.ndim(), std::min (out.ndim(), std::min (out2.ndim(), out3.ndim()))));
        for (size_t n = from_axis; n < to_axis; ++n)
          out[n] = out2[n] = out3[n] = in[n];
      }

    //! reset all coordinates to zero.
    template <class VoxelType>
      void voxel_reset (VoxelType& vox) {
        for (size_t n = 0; n < vox.ndim(); ++n)
          vox[n] = 0;
      }

  }
}

#endif




