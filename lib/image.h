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
#include "image/copy.h"
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

          size_t offset () const { 
            return offset_;
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

          std::string save (const std::string& filename, bool use_multi_threading = true) const 
          {
            Voxel in (*this);
            Image::Header header;
            header.info() = info();
            Image::Buffer<value_type> buffer_out (filename, header);
            auto out = buffer_out.voxel();
            if (use_multi_threading) 
              Image::threaded_copy (in, out);
            else 
              Image::copy (in, out);
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

    template <class InputVoxelType, class OutputVoxelType>
      void voxel_assign (OutputVoxelType& out, const InputVoxelType& in, const std::vector<size_t>& axes) {
        for (size_t n = 0; n < axes.size(); ++n)
          out[axes[n]] = in[axes[n]];
      }

    template <class InputVoxelType, class OutputVoxelType, class OutputVoxelType2>
      void voxel_assign2 (OutputVoxelType& out, OutputVoxelType2& out2, const InputVoxelType& in, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) {
        to_axis = std::min (to_axis, std::min (in.ndim(), std::min (out.ndim(), out2.ndim())));
        for (size_t n = from_axis; n < to_axis; ++n)
          out[n] = out2[n] = in[n];
      }

    template <class InputVoxelType, class OutputVoxelType, class OutputVoxelType2>
      void voxel_assign2 (OutputVoxelType& out, OutputVoxelType2& out2, const InputVoxelType& in, const std::vector<size_t>& axes) {
        for (size_t n = 0; n < axes.size(); ++n)
          out[axes[n]] = out2[axes[n]] = in[axes[n]];
      }

    template <class InputVoxelType, class OutputVoxelType, class OutputVoxelType2, class OutputVoxelType3>
      void voxel_assign3 (OutputVoxelType& out, OutputVoxelType2& out2, OutputVoxelType3& out3, const InputVoxelType& in, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) {
        to_axis = std::min (to_axis, std::min (in.ndim(), std::min (out.ndim(), std::min (out2.ndim(), out3.ndim()))));
        for (size_t n = from_axis; n < to_axis; ++n)
          out[n] = out2[n] = out3[n] = in[n];
      }

    template <class InputVoxelType, class OutputVoxelType, class OutputVoxelType2 , class OutputVoxelType3>
      void voxel_assign3 (OutputVoxelType& out, OutputVoxelType2& out2, OutputVoxelType3& out3, const InputVoxelType& in, const std::vector<size_t>& axes) {
        for (size_t n = 0; n < axes.size(); ++n)
          out[axes[n]] = out2[axes[n]] = out3[axes[n]] = in[axes[n]];
      }

    template <class InputVoxelType, class OutputVoxelType, class OutputVoxelType2 , class OutputVoxelType3, class OutputVoxelType4>
      void voxel_assign4 (OutputVoxelType& out, OutputVoxelType2& out2, OutputVoxelType3& out3, OutputVoxelType4& out4, const InputVoxelType& in, const std::vector<size_t>& axes) {
        for (size_t n = 0; n < axes.size(); ++n)
          out[axes[n]] = out2[axes[n]] = out3[axes[n]] = out4[axes[n]] = in[axes[n]];
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

#ifndef __image_buffer_h__
#define __image_buffer_h__

#include <functional>
#include <type_traits>

#include "debug.h"
#include "get_set.h"
#include "image/header.h"
#include "image/adapter/info.h"
#include "image/stride.h"
#include "image/value.h"
#include "image/position.h"

namespace MR
{
  namespace Image
  {

    //! \cond skip

    template <class BufferType> class Voxel;

    namespace
    {

      // rounding to be applied during conversion:

      // any -> floating-point
      template <typename TypeOUT, typename TypeIN>
        inline typename std::enable_if<std::is_floating_point<TypeOUT>::value, TypeOUT>::type 
        round_func (TypeIN in, typename std::enable_if<std::is_arithmetic<TypeIN>::value>::type* = nullptr) {
          return in;
        }

      // integer -> integer
      template <typename TypeOUT, typename TypeIN>
        inline typename std::enable_if<std::is_integral<TypeOUT>::value, TypeOUT>::type 
        round_func (TypeIN in, typename std::enable_if<std::is_integral<TypeIN>::value>::type* = nullptr) {
          return in;
        }

      // floating-point -> integer
      template <typename TypeOUT, typename TypeIN>
        inline typename std::enable_if<std::is_integral<TypeOUT>::value, TypeOUT>::type 
        round_func (TypeIN in, typename std::enable_if<std::is_floating_point<TypeIN>::value>::type* = nullptr) {
          return std::isfinite (in) ? std::round (in) : TypeOUT (0);
        }

      // complex -> complex
      template <typename TypeOUT, typename TypeIN>
        inline typename std::enable_if<std::is_same<std::complex<typename TypeOUT::value_type>, TypeOUT>::value, TypeOUT>::type 
        round_func (TypeIN in, typename std::enable_if<std::is_same<std::complex<typename TypeIN::value_type>, TypeIN>::value>::type* = nullptr) {
          return TypeOUT (in);
        }

      // real -> complex
      template <typename TypeOUT, typename TypeIN>
        inline typename std::enable_if<std::is_same<std::complex<typename TypeOUT::value_type>, TypeOUT>::value, TypeOUT>::type 
        round_func (TypeIN in, typename std::enable_if<std::is_arithmetic<TypeIN>::value>::type* = nullptr) {
          return round_func<typename TypeOUT::value_type> (in);
        }

      // complex -> real
      template <typename TypeOUT, typename TypeIN>
        inline typename std::enable_if<std::is_arithmetic<TypeOUT>::value, TypeOUT>::type 
        round_func (TypeIN in, typename std::enable_if<std::is_same<std::complex<typename TypeIN::value_type>, TypeIN>::value>::type* = nullptr) {
          return round_func<TypeOUT> (in.real());
        }




      // for single-byte types:

      template <typename RAMType, typename DiskType> 
        RAMType __get (const void* data, size_t i) {
          return round_func<RAMType> (MR::get<DiskType> (data, i)); 
        }

      template <typename RAMType, typename DiskType> 
        void __put (RAMType val, void* data, size_t i) {
          return MR::put<DiskType> (round_func<DiskType> (val), data, i); 
        }

      // for little-endian multi-byte types:

      template <typename RAMType, typename DiskType> 
        RAMType __getLE (const void* data, size_t i) {
          return round_func<RAMType> (MR::getLE<DiskType> (data, i)); 
        }

      template <typename RAMType, typename DiskType> 
        void __putLE (RAMType val, void* data, size_t i) {
          return MR::putLE<DiskType> (round_func<DiskType> (val), data, i); 
        }


      // for big-endian multi-byte types:

      template <typename RAMType, typename DiskType> 
        RAMType __getBE (const void* data, size_t i) {
          return round_func<RAMType> (MR::getBE<DiskType> (data, i)); 
        }

      template <typename RAMType, typename DiskType> 
        void __putBE (RAMType val, void* data, size_t i) {
          return MR::putBE<DiskType> (round_func<DiskType> (val), data, i); 
        }

    }

    // \endcond





    template <typename ValueType> 
      class Buffer : public ConstHeader
    {
      public:
        //! construct a Buffer object to access the data in the image specified
        Buffer (const std::string& image_name, bool readwrite = false) :
          ConstHeader (image_name) {
            assert (handler_);
            handler_->set_readwrite (readwrite);
            handler_->open();
            set_get_put_functions ();
          }

        //! construct a Buffer object to access the data in \a header
        Buffer (const Header& header, bool readwrite = false) :
          ConstHeader (header) {
            handler_ = header.__get_handler();
            assert (handler_);
            handler_->set_readwrite (readwrite);
            handler_->open();
            set_get_put_functions ();
          }

        //! construct a Buffer object to access the same data as a buffer using a different data type.
        template <typename OtherValueType>
          Buffer (const Buffer<OtherValueType>& buffer) :
            ConstHeader (buffer) {
              handler_ = buffer.__get_handler();
              assert (handler_);
              set_get_put_functions ();
            }

        //! construct a Buffer object to access the same data as another buffer.
        Buffer (const Buffer<ValueType>& that) :
          ConstHeader (that) {
            handler_ = that.__get_handler();
            assert (handler_);
            set_get_put_functions ();
          }

        //! construct a Buffer object to create and access the image specified
        Buffer (const std::string& image_name, const Header& template_header) :
          ConstHeader (template_header) {
            create (image_name);
            assert (handler_);
            handler_->open();
            set_get_put_functions ();
          }

        typedef ValueType value_type;
        typedef typename Image::Voxel<Buffer> voxel_type;

        voxel_type voxel() { return voxel_type (*this); }

        value_type get_value (size_t offset) const {
          ssize_t nseg (offset / handler_->segment_size());
          return scale_from_storage (get_func (handler_->segment (nseg), offset - nseg*handler_->segment_size()));
        }

        void set_value (size_t offset, value_type val) {
          ssize_t nseg (offset / handler_->segment_size());
          put_func (scale_to_storage (val), handler_->segment (nseg), offset - nseg*handler_->segment_size());
        }

        friend std::ostream& operator<< (std::ostream& stream, const Buffer& V) {
          stream << "data for image \"" << V.name() << "\": " + str (Image::voxel_count (V))
            + " voxels in " + V.datatype().specifier() + " format, stored in " + str (V.handler_->nsegments())
            + " segments of size " + str (V.handler_->segment_size())
            + ", at addresses [ ";
          for (size_t n = 0; n < V.handler_->nsegments(); ++n)
            stream << str ((void*) V.handler_->segment(n)) << " ";
          stream << "]";
          return stream;
        }

      protected:
        template <class InfoType> 
          Buffer& operator= (const InfoType&) { assert (0); return *this; }

        std::function<value_type(const void*,size_t)> get_func;
        std::function<void(value_type,void*,size_t)> put_func;

        void set_get_put_functions () {

          switch (datatype() ()) {
            case DataType::Bit:
              get_func = __get<value_type,bool>;
              put_func = __put<value_type,bool>;
              return;
            case DataType::Int8:
              get_func = __get<value_type,int8_t>;
              put_func = __put<value_type,int8_t>;
              return;
            case DataType::UInt8:
              get_func = __get<value_type,uint8_t>;
              put_func = __put<value_type,uint8_t>;
              return;
            case DataType::Int16LE:
              get_func = __getLE<value_type,int16_t>;
              put_func = __putLE<value_type,int16_t>;
              return;
            case DataType::UInt16LE:
              get_func = __getLE<value_type,uint16_t>;
              put_func = __putLE<value_type,uint16_t>;
              return;
            case DataType::Int16BE:
              get_func = __getBE<value_type,int16_t>;
              put_func = __putBE<value_type,int16_t>;
              return;
            case DataType::UInt16BE:
              get_func = __getBE<value_type,uint16_t>;
              put_func = __putBE<value_type,uint16_t>;
              return;
            case DataType::Int32LE:
              get_func = __getLE<value_type,int32_t>;
              put_func = __putLE<value_type,int32_t>;
              return;
            case DataType::UInt32LE:
              get_func = __getLE<value_type,uint32_t>;
              put_func = __putLE<value_type,uint32_t>;
              return;
            case DataType::Int32BE:
              get_func = __getBE<value_type,int32_t>;
              put_func = __putBE<value_type,int32_t>;
              return;
            case DataType::UInt32BE:
              get_func = __getBE<value_type,uint32_t>;
              put_func = __putBE<value_type,uint32_t>;
              return;
            case DataType::Int64LE:
              get_func = __getLE<value_type,int64_t>;
              put_func = __putLE<value_type,int64_t>;
              return;
            case DataType::UInt64LE:
              get_func = __getLE<value_type,uint64_t>;
              put_func = __putLE<value_type,uint64_t>;
              return;
            case DataType::Int64BE:
              get_func = __getBE<value_type,int64_t>;
              put_func = __putBE<value_type,int64_t>;
              return;
            case DataType::UInt64BE:
              get_func = __getBE<value_type,uint64_t>;
              put_func = __putBE<value_type,uint64_t>;
              return;
            case DataType::Float32LE:
              get_func = __getLE<value_type,float>;
              put_func = __putLE<value_type,float>;
              return;
            case DataType::Float32BE:
              get_func = __getBE<value_type,float>;
              put_func = __putBE<value_type,float>;
              return;
            case DataType::Float64LE:
              get_func = __getLE<value_type,double>;
              put_func = __putLE<value_type,double>;
              return;
            case DataType::Float64BE:
              get_func = __getBE<value_type,double>;
              put_func = __putBE<value_type,double>;
              return;
            case DataType::CFloat32LE:
              get_func = __getLE<value_type,cfloat>;
              put_func = __putLE<value_type,cfloat>;
              return;
            case DataType::CFloat32BE:
              get_func = __getBE<value_type,cfloat>;
              put_func = __putBE<value_type,cfloat>;
              return;
            case DataType::CFloat64LE:
              get_func = __getLE<value_type,cdouble>;
              put_func = __putLE<value_type,cdouble>;
              return;
            case DataType::CFloat64BE:
              get_func = __getBE<value_type,cdouble>;
              put_func = __putBE<value_type,cdouble>;
              return;
            default:
              throw Exception ("invalid data type in image header");
          }
        }

        value_type scale_from_storage (value_type val) const {
          return value_type(intensity_offset()) + value_type(intensity_scale()) * val;
        }

        value_type scale_to_storage (value_type val) const   {
          return (val - value_type(intensity_offset())) / value_type(intensity_scale());
        }

    };




  }
}

#endif



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

#ifndef __image_buffer_preload_h__
#define __image_buffer_preload_h__

#include "debug.h"
#include "image/buffer.h"
#include "image/threaded_copy.h"

namespace MR
{
  namespace Image
  {


    template <typename ValueType>
      class BufferPreload : public Buffer<ValueType>
    {
      public:
        BufferPreload (const std::string& image_name) :
          Buffer<value_type> (image_name),
          data_ (NULL) {
            init();
          }

        BufferPreload (const std::string& image_name, const Image::Stride::List& desired_strides) :
          Buffer<value_type> (image_name),
          data_ (NULL) {
            init (desired_strides);
          }

        BufferPreload (const Header& header) :
          Buffer<value_type> (header),
          data_ (NULL) {
            init();
          }

        BufferPreload (const Header& header, const Image::Stride::List& desired_strides) :
          Buffer<value_type> (header),
          data_ (NULL) {
            init (desired_strides);
          }

        BufferPreload (const Buffer<ValueType>& buffer) :
          Buffer<value_type> (buffer),
          data_ (NULL) {
            init();
          }

        ~BufferPreload () {
          if (!handler_)
            delete [] data_;
        }

        typedef ValueType value_type;
        typedef Image::Voxel<BufferPreload> voxel_type;

        voxel_type voxel() { return voxel_type (*this); }

        value_type get_value (size_t index) const {
          return data_[index];
        }

        void set_value (size_t index, value_type val) {
          data_[index] = val;
        }

        value_type* address (size_t index) const {
          return &data_[index];
        }


        friend std::ostream& operator<< (std::ostream& stream, const BufferPreload& V) {
          stream << "preloaded data for image \"" << V.name() << "\": " + str (Image::voxel_count (V))
            + " voxels in " + V.datatype().specifier() + " format, stored at address " + str ((void*) V.data_);
          return stream;
        }

        using Buffer<value_type>::name;
        using Buffer<value_type>::datatype;
        using Buffer<value_type>::intensity_offset;
        using Buffer<value_type>::intensity_scale;

      protected:
        value_type* data_;
        using Buffer<value_type>::handler_;

        template <class Set> BufferPreload& operator= (const Set& H) { assert (0); return *this; }

        void init (const Stride::List& desired_strides) {
          Stride::List new_strides = Image::Stride::get_nearest_match (static_cast<ConstHeader&> (*this), desired_strides);

          if (new_strides == Image::Stride::get (static_cast<ConstHeader&> (*this))) {
            init();
            return;
          }

          Stride::List old_strides = Stride::get (*this);
          Stride::set (static_cast<Header&> (*this), new_strides);
          voxel_type dest_vox (*this);

          Stride::set (static_cast<Header&> (*this), old_strides);
          do_load (dest_vox);
          Stride::set (static_cast<Header&> (*this), new_strides);
        }


        void init () {
          assert (handler_);
          assert (handler_->nsegments());

          if (handler_->nsegments() == 1 && 
              datatype() == DataType::from<value_type>() &&
              intensity_offset() == 0.0 && intensity_scale() == 1.0) {
            INFO ("data in \"" + name() + "\" already in required format - mapping as-is");
            data_ = reinterpret_cast<value_type*> (handler_->segment (0));
            return;
          }

          voxel_type dest (*this);
          do_load (dest);
        }

        template <class VoxType>
          void do_load (VoxType& destination) {
            INFO ("data for image \"" + name() + "\" will be loaded into memory");
            data_ = new value_type [Image::voxel_count (*this)];
            Buffer<value_type>& filedata (*this);
            typename Buffer<value_type>::voxel_type src (filedata);
            Image::threaded_copy_with_progress_message ("loading data for image \"" + name() + "\"...", src, destination);

            this->Info::datatype_ = DataType::from<value_type>();
            handler_ = NULL;
          }

    };

    template <> inline bool BufferPreload<bool>::get_value (size_t index) const {
      return get<bool> (data_, index);
    }

    template <> inline void BufferPreload<bool>::set_value (size_t index, bool val) {
      put<bool> (val, data_, index);
    }


  }
}

#endif




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

#ifndef __image_buffer_scratch_h__
#define __image_buffer_scratch_h__

#include "bitset.h"
#include "debug.h"
#include "image/info.h"
#include "image/stride.h"
#include "image/voxel.h"
#include "image/adapter/info.h"

namespace MR
{
  namespace Image
  {


    template <typename ValueType>
      class BufferScratch : public ConstInfo
    {
      public:
        template <class Template>
          BufferScratch (const Template& info) :
            ConstInfo (info),
            data_ (Image::voxel_count (*this)) {
              datatype_ = DataType::from<value_type>();
              zero();
            }

        template <class Template>
          BufferScratch (const Template& info, const std::string& label) :
            ConstInfo (info),
            data_ (Image::voxel_count (*this)) {
              datatype_ = DataType::from<value_type>();
              zero();
              name_ = label;
            }

        typedef ValueType value_type;
        typedef Image::Voxel<BufferScratch> voxel_type;

        voxel_type voxel() { return voxel_type (*this); }

        void zero () {
          memset (address(0), 0, data_.size()*sizeof(value_type));
        }

        value_type get_value (size_t index) const {
          return data_[index];
        }

        void set_value (size_t index, value_type val) {
          data_[index] = val;
        }

        const value_type* address (size_t index) const {
          return &data_[index];
        }

        value_type* address (size_t index) {
          return &data_[index];
        }

        friend std::ostream& operator<< (std::ostream& stream, const BufferScratch& V) {
          stream << "scratch image data \"" << V.name() << "\": " + str (Image::voxel_count (V))
            + " voxels in " + V.datatype().specifier() + " format, stored at address " + str ((void*) V.address(0));
          return stream;
        }

      protected:
        std::vector<value_type> data_;
    };





    template <>
    class BufferScratch<bool> : public ConstInfo
    {
      public:
        template <class Template>
        BufferScratch (const Template& info) :
            ConstInfo (info),
            data_ ((Image::voxel_count (*this)+7)/8, 0)
        {
          Info::datatype_ = DataType::Bit;
        }

        template <class Template>
        BufferScratch (const Template& info, const std::string& label) :
            ConstInfo (info),
            data_ ((Image::voxel_count (*this)+7)/8, 0)
        {
          Info::datatype_ = DataType::Bit;
          name_ = label;
        }

        typedef bool value_type;
        typedef Image::Voxel<BufferScratch> voxel_type;

        voxel_type voxel() { return voxel_type (*this); }

        void zero () {
          std::fill (data_.begin(), data_.end(), 0);
        }

        bool get_value (size_t index) const {
          return get<bool> (address(), index);
        }

        void set_value (size_t index, bool val) {
          put<bool> (val, address(), index);
        }

        void* address () { return &data_[0]; }
        const void* address () const { return &data_[0]; }

        friend std::ostream& operator<< (std::ostream& stream, const BufferScratch& V) {
          stream << "scratch image data \"" << V.name() << "\": " + str (Image::voxel_count (V))
            + " voxels in boolean format (" << V.data_.size() << " bytes), stored at address " + str ((void*) V.address());
          return stream;
        }

      protected:
        std::vector<uint8_t> data_;
    };



  }
}

#endif




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

#ifndef __image_misc_h__
#define __image_misc_h__

#include "types.h"
#include "datatype.h"

namespace MR
{
  namespace Image
  {

    //! returns the number of voxel in the data set, or a relevant subvolume
    template <class InfoType> 
      inline size_t voxel_count (const InfoType& in, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max())
    {
      if (to_axis > in.ndim()) to_axis = in.ndim();
      assert (from_axis < to_axis);
      size_t fp = 1;
      for (size_t n = from_axis; n < to_axis; ++n)
        fp *= in.dim (n);
      return fp;
    }

    //! returns the number of voxel in the relevant subvolume of the data set
    template <class InfoType> 
      inline size_t voxel_count (const InfoType& in, const char* specifier)
    {
      size_t fp = 1;
      for (size_t n = 0; n < in.ndim(); ++n)
        if (specifier[n] != ' ') fp *= in.dim (n);
      return fp;
    }

    //! returns the number of voxel in the relevant subvolume of the data set
    template <class InfoType> 
      inline int64_t voxel_count (const InfoType& in, const std::vector<size_t>& axes)
    {
      int64_t fp = 1;
      for (size_t n = 0; n < axes.size(); ++n) {
        assert (axes[n] < in.ndim());
        fp *= in.dim (axes[n]);
      }
      return fp;
    }

    inline int64_t footprint (int64_t count, DataType dtype) {
      return dtype == DataType::Bit ? (count+7)/8 : count*dtype.bytes();
    }

    //! returns the memory footprint of a DataSet
    template <class InfoType> 
      inline int64_t footprint (const InfoType& in, size_t from_dim = 0, size_t up_to_dim = std::numeric_limits<size_t>::max()) {
      return footprint (Image::voxel_count (in, from_dim, up_to_dim), in.datatype());
    }

    //! returns the memory footprint of a DataSet
    template <class InfoType> 
      inline int64_t footprint (const InfoType& in, const char* specifier) {
      return footprint (Image::voxel_count (in, specifier), in.datatype());
    }


    //! \cond skip
    namespace
    {
      template <typename T> inline bool is_complex__ ()
      {
        return false;
      }
      template <> inline bool is_complex__<cfloat> ()
      {
        return true;
      }
      template <> inline bool is_complex__<cdouble> ()
      {
        return true;
      }
    }
    //! \endcond




    //! return whether the InfoType contains complex data
    template <class InfoType> 
      inline bool is_complex (const InfoType&)
    {
      typedef typename InfoType::value_type T;
      return is_complex__<T> ();
    }

    template <class InfoType1, class InfoType2> 
      inline bool dimensions_match (const InfoType1& in1, const InfoType2& in2)
    {
      if (in1.ndim() != in2.ndim()) return false;
      for (size_t n = 0; n < in1.ndim(); ++n)
        if (in1.dim (n) != in2.dim (n)) return false;
      return true;
    }

    template <class InfoType1, class InfoType2> 
      inline bool dimensions_match (const InfoType1& in1, const InfoType2& in2, size_t from_axis, size_t to_axis)
    {
      assert (from_axis < to_axis);
      if (to_axis > in1.ndim() || to_axis > in2.ndim()) return false;
      for (size_t n = from_axis; n < to_axis; ++n)
        if (in1.dim (n) != in2.dim (n)) return false;
      return true;
    }

    template <class InfoType1, class InfoType2> 
      inline bool dimensions_match (const InfoType1& in1, const InfoType2& in2, const std::vector<size_t>& axes)
    {
      for (size_t n = 0; n < axes.size(); ++n) {
        if (in1.ndim() <= axes[n] || in2.ndim() <= axes[n]) return false;
        if (in1.dim (axes[n]) != in2.dim (axes[n])) return false;
      }
      return true;
    }

    template <class InfoType1, class InfoType2> 
      inline void check_dimensions (const InfoType1& in1, const InfoType2& in2)
    {
      if (!dimensions_match (in1, in2))
        throw Exception ("dimension mismatch between \"" + in1.name() + "\" and \"" + in2.name() + "\"");
    }

    template <class InfoType1, class InfoType2> 
      inline void check_dimensions (const InfoType1& in1, const InfoType2& in2, size_t from_axis, size_t to_axis)
    {
      if (!dimensions_match (in1, in2, from_axis, to_axis))
        throw Exception ("dimension mismatch between \"" + in1.name() + "\" and \"" + in2.name() + "\"");
    }

    template <class InfoType1, class InfoType2> 
      inline void check_dimensions (const InfoType1& in1, const InfoType2& in2, const std::vector<size_t>& axes)
    {
      if (!dimensions_match (in1, in2, axes))
        throw Exception ("dimension mismatch between \"" + in1.name() + "\" and \"" + in2.name() + "\"");
    }



    template <class InfoType>
      inline void squeeze_dim (InfoType& in, size_t from_axis = 3) 
      {
        size_t n = in.ndim();
        while (in.dim(n-1) <= 1 && n > from_axis) --n;
        in.set_ndim (n);
      }


    //! \cond skip
    namespace {

      template <class VoxelType>
        struct __assign_pos_axis_range
        {
          template <class VoxelType2>
            void operator() (VoxelType2& out) const {
              const size_t max_axis = std::min (to_axis, std::min (ref.ndim(), out.ndim()));
              for (size_t n = from_axis; n < max_axis; ++n)
                out[n] = ref[n];
            }
          const VoxelType& ref;
          const size_t from_axis, to_axis;
        };


      template <class VoxelType, typename IntType>
        struct __assign_pos_axes
        {
          template <class VoxelType2>
            void operator() (VoxelType2& out) const {
              for (auto a : axes) 
                out[a] = ref[a];
            }
          const VoxelType& ref;
          const std::vector<IntType> axes;
        };
    }

    //! \endcond


    //! returns a functor to set the position in ref to other voxels
    template <class VoxelType>
      inline __assign_pos_axis_range<VoxelType> 
      assign_pos (const VoxelType& reference, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) 
      {
        return { reference, from_axis, to_axis };
      }

    //! returns a functor to set the position in ref to other voxels
    template <class VoxelType, typename IntType>
      inline __assign_pos_axes<VoxelType, IntType> 
      assign_pos (const VoxelType& reference, const std::vector<IntType>& axes) 
      {
        return { reference, axes };
      }

    //! returns a functor to set the position in ref to other voxels
    template <class VoxelType, typename IntType>
      inline __assign_pos_axes<VoxelType, IntType> 
      assign_pos (const VoxelType& reference, const std::vector<IntType>&& axes) 
      {
        return assign_pos (reference, axes);
      }


  }
}

#endif

