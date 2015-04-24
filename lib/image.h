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

#ifndef __image_h__
#define __image_h__

#include <functional>
#include <type_traits>
#include <tuple>

#include "debug.h"
#include "get_set.h"
#include "header.h"
#include "image_helpers.h"
#include "algo/copy.h"
#include "algo/threaded_copy.h"

namespace MR
{

  template <typename ValueType>
    class Image {
      public:
        typedef ValueType value_type;
        class Buffer;

        Image () = delete;
        Image (const Image&) = default;
        Image (Image&&) = default;
        Image& operator= (const Image&) = default;
        ~Image();

        //! used internally to instantiate Image objects
        Image (const std::shared_ptr<const Buffer>&, const Stride::List& = Stride::List());

        const Header& header () const { return *buffer; }

        const std::string& name() const { return buffer->name(); }
        const Math::Matrix<default_type>& transform() const { return buffer->transform(); }

        size_t  ndim () const { return buffer->ndim(); }
        ssize_t size (size_t axis) const { return buffer->size (axis); }
        default_type voxsize (size_t axis) const { return buffer->voxsize (axis); }
        ssize_t stride (size_t axis) const { return strides[axis]; }

        //! offset to current voxel from start of data
        size_t offset () const { return data_offset; }

        //! get position of current voxel location along \a axis
        ssize_t index (size_t axis) const { return get_voxel_position (axis); }
        //! get/set position of current voxel location along \a axis
        Helper::VoxelIndex<Image> index (size_t axis) { return { *this, axis }; }

        //! get voxel value at current location
        ValueType value () const { return get_voxel_value (); }
        //! get/set voxel value at current location
        Helper::VoxelValue<Image> value () { return { *this }; }

        //! use for debugging
        friend std::ostream& operator<< (std::ostream& stream, const Image& V) {
          stream << "\"" << V.name() << "\", datatype " << DataType::from<Image::value_type>().specifier() << ", index [ ";
          for (size_t n = 0; n < V.ndim(); ++n) stream << V.index(n) << " ";
          stream << "], current offset = " << V.offset() << ", value = " << V.value();
          if (!V.data_pointer) stream << " (using indirect IO)";
          else stream << " (using direct IO, data at " << (void*) V.data_pointer << ")";
          return stream;
        }

        //! write out the contents of the image to file
        std::string save (const std::string& filename, bool use_multi_threading = true) const 
        {
          /*
             Image in (*this);
             Image::Header header;
             header.info() = info();
             Image::Buffer<value_type> buffer_out (filename, header);
             auto out = buffer_out.voxel();
             if (use_multi_threading) 
             Image::threaded_copy (in, out);
             else 
             Image::copy (in, out);
             return buffer_out.__get_handler()->files[0].name;
             */
          return std::string();
        }

        //! return a new Image using direct IO
        /*! this will preload the data into RAM if the datatype on file doesn't
         * match that on file (or if any scaling is applied to the data). The
         * optional \a with_strides argument is used to additionally enforce
         * preloading if the strides aren't compatible with those specified. 
         *
         * Example:
         * \code
         * auto image = Header::open (argument[0]).get_image().with_direct_io();
         * \endcode 
         * \note this invalidate the invoking Image - do not use the original
         * image in subsequent code.*/
         const Image with_direct_io (Stride::List with_strides = Stride::List()) const;


        //! display the contents of the image in MRView
        void display () const {
          std::string filename = save ("-");
          CONSOLE ("displaying image " + filename);
          if (system (("bash -c \"mrview " + filename + "\"").c_str()))
            WARN (std::string("error invoking viewer: ") + strerror(errno));
        }

        //! return RAM address of current voxel
        /*! \note this will only work if image access is direct (i.e. for a
         * scratch image, with preloading, or when the data type is native and
         * without scaling. */
        ValueType* address () const { return data_pointer ? data_pointer + data_offset : nullptr; }

      protected:
        //! shared reference to header/buffer
        const std::shared_ptr<const Buffer> buffer;
        //! pointer to data address whether in RAM or MMap
        ValueType * const data_pointer;
        //! voxel indices
        std::vector<ssize_t> x;
        //! voxel indices
        const Stride::List strides;
        //! offset to currently pointed-to voxel
        size_t data_offset;

        ValueType get_voxel_value () const {
          if (data_pointer) return data_pointer[data_offset];
          return buffer->get_value (data_offset);
        }

        void set_voxel_value (ValueType val) {
          if (data_pointer) data_pointer[data_offset] = val;
          else buffer->set_value (data_offset, val);
        }

        ssize_t get_voxel_position (size_t axis) const {
          return x[axis];
        }
        void set_voxel_position (size_t axis, ssize_t position) {
          data_offset += stride (axis) * (position - x[axis]);
          x[axis] = position;
        }
        void move_voxel_position (size_t axis, ssize_t increment) {
          data_offset += stride (axis) * increment;
          x[axis] += increment;
        }

        const Image with_direct_io (Stride::List with_strides = Stride::List()); // do not use this function with a non-const Image

        friend class Helper::VoxelIndex<Image>;
        friend class Helper::VoxelValue<Image>;
    };








    template <typename ValueType> 
      class Image<ValueType>::Buffer : public Header
    {
      public:
        //! construct a Buffer object to access the data in the image specified
        Buffer (const Header& H, bool read_write_if_existing = false, bool direct_io = false, Stride::List strides = Stride::List());
        Buffer (Buffer&&) = default;
        Buffer& operator= (const Buffer&) = delete;
        Buffer (const Buffer& b) : 
          Header (b), get_func (b.get_func), put_func (b.put_func) { }


        ValueType get_value (size_t offset) const {
          ssize_t nseg = offset / io->segment_size();
          return get_func (io->segment (nseg), offset - nseg*io->segment_size(), intensity_offset(), intensity_scale());
        }

        void set_value (size_t offset, ValueType val) const {
          ssize_t nseg = offset / io->segment_size();
          put_func (val, io->segment (nseg), offset - nseg*io->segment_size(), intensity_offset(), intensity_scale());
        }

        mutable std::unique_ptr<ValueType[]> data_buffer;
        ValueType* get_data_pointer () const;

        ImageIO::Base* get_io () const { return io.get(); }
        void grab_io (const Buffer& b) const { io = std::move (b.io); }

      protected:
        std::function<ValueType(const void*,size_t,default_type,default_type)> get_func;
        std::function<void(ValueType,void*,size_t,default_type,default_type)> put_func;

        void set_get_put_functions (DataType datatype);
    };













    //! \cond skip


    // functions needed for conversion to/from storage:
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



      // apply scaling from storage:
      template <typename DiskType>
        inline typename std::enable_if<std::is_arithmetic<DiskType>::value, default_type>::type 
        scale_from_storage (DiskType val, default_type offset, default_type scale) {
          return offset + scale * val;
        }

      template <typename DiskType>
        inline typename std::enable_if<std::is_same<std::complex<typename DiskType::value_type>, DiskType>::value, DiskType>::type 
        scale_from_storage (DiskType val, default_type offset, default_type scale) {
          return typename DiskType::value_type (offset) + typename DiskType::value_type (scale) * val;
        }

      // apply scaling to storage:
      template <typename DiskType>
        inline typename std::enable_if<std::is_arithmetic<DiskType>::value, default_type>::type 
        scale_to_storage (DiskType val, default_type offset, default_type scale) {
          return (val - offset) / scale;
        }

      template <typename DiskType>
        inline typename std::enable_if<std::is_same<std::complex<typename DiskType::value_type>, DiskType>::value, DiskType>::type 
        scale_to_storage (DiskType val, default_type offset, default_type scale) {
          return (val - typename DiskType::value_type (offset)) / typename DiskType::value_type (scale);
        }



      // for single-byte types:

      template <typename RAMType, typename DiskType> 
        RAMType __get (const void* data, size_t i, default_type offset, default_type scale) {
          return round_func<RAMType> (scale_from_storage (MR::get<DiskType> (data, i), offset, scale)); 
        }

      template <typename RAMType, typename DiskType> 
        void __put (RAMType val, void* data, size_t i, default_type offset, default_type scale) {
          return MR::put<DiskType> (round_func<DiskType> (scale_to_storage (val, offset, scale)), data, i); 
        }

      // for little-endian multi-byte types:

      template <typename RAMType, typename DiskType> 
        RAMType __getLE (const void* data, size_t i, default_type offset, default_type scale) {
          return round_func<RAMType> (scale_from_storage (MR::getLE<DiskType> (data, i), offset, scale)); 
        }

      template <typename RAMType, typename DiskType> 
        void __putLE (RAMType val, void* data, size_t i, default_type offset, default_type scale) {
          return MR::putLE<DiskType> (round_func<DiskType> (scale_to_storage (val, offset, scale)), data, i); 
        }


      // for big-endian multi-byte types:

      template <typename RAMType, typename DiskType> 
        RAMType __getBE (const void* data, size_t i, default_type offset, default_type scale) {
          return round_func<RAMType> (scale_from_storage (MR::getBE<DiskType> (data, i), offset, scale)); 
        }

      template <typename RAMType, typename DiskType> 
        void __putBE (RAMType val, void* data, size_t i, default_type offset, default_type scale) {
          return MR::putBE<DiskType> (round_func<DiskType> (scale_to_storage (val, offset, scale)), data, i); 
        }





      // lightweight struct to copy data into:
      template <typename ValueType>
      struct TmpImage {
        typedef ValueType value_type;

        const typename Image<ValueType>::Buffer& b;
        value_type* const data;
        std::vector<ssize_t> x;
        const Stride::List& strides;
        size_t offset;

        const std::string name () const { return "direct IO buffer"; }
        size_t ndim () const { return b.ndim(); }
        ssize_t size (size_t axis) const { return b.size(axis); }
        ssize_t stride (size_t axis) const { return strides[axis]; }
        ssize_t index (size_t axis) const { return x[axis]; }
        Helper::VoxelIndex<TmpImage> index (size_t axis) { return { *this, axis }; }
        value_type value () const { return data[offset]; }
        Helper::VoxelValue<TmpImage> value () { return { *this }; }

        void set_voxel_value (ValueType val) { data[offset] = val; }
        value_type get_voxel_value () const { return data[offset]; }
        ssize_t get_voxel_position (size_t axis) const { return x[axis]; }
        void set_voxel_position (size_t axis, ssize_t position) { offset += stride (axis) * (position - x[axis]); x[axis] = position; }
        void move_voxel_position (size_t axis, ssize_t increment) { offset += stride (axis) * increment; x[axis] += increment; }
      };

    }






    template <typename ValueType> 
      void Image<ValueType>::Buffer::set_get_put_functions (DataType datatype) {

        switch (datatype()) {
          case DataType::Bit:
            get_func = __get<ValueType,bool>;
            put_func = __put<ValueType,bool>;
            return;
          case DataType::Int8:
            get_func = __get<ValueType,int8_t>;
            put_func = __put<ValueType,int8_t>;
            return;
          case DataType::UInt8:
            get_func = __get<ValueType,uint8_t>;
            put_func = __put<ValueType,uint8_t>;
            return;
          case DataType::Int16LE:
            get_func = __getLE<ValueType,int16_t>;
            put_func = __putLE<ValueType,int16_t>;
            return;
          case DataType::UInt16LE:
            get_func = __getLE<ValueType,uint16_t>;
            put_func = __putLE<ValueType,uint16_t>;
            return;
          case DataType::Int16BE:
            get_func = __getBE<ValueType,int16_t>;
            put_func = __putBE<ValueType,int16_t>;
            return;
          case DataType::UInt16BE:
            get_func = __getBE<ValueType,uint16_t>;
            put_func = __putBE<ValueType,uint16_t>;
            return;
          case DataType::Int32LE:
            get_func = __getLE<ValueType,int32_t>;
            put_func = __putLE<ValueType,int32_t>;
            return;
          case DataType::UInt32LE:
            get_func = __getLE<ValueType,uint32_t>;
            put_func = __putLE<ValueType,uint32_t>;
            return;
          case DataType::Int32BE:
            get_func = __getBE<ValueType,int32_t>;
            put_func = __putBE<ValueType,int32_t>;
            return;
          case DataType::UInt32BE:
            get_func = __getBE<ValueType,uint32_t>;
            put_func = __putBE<ValueType,uint32_t>;
            return;
          case DataType::Int64LE:
            get_func = __getLE<ValueType,int64_t>;
            put_func = __putLE<ValueType,int64_t>;
            return;
          case DataType::UInt64LE:
            get_func = __getLE<ValueType,uint64_t>;
            put_func = __putLE<ValueType,uint64_t>;
            return;
          case DataType::Int64BE:
            get_func = __getBE<ValueType,int64_t>;
            put_func = __putBE<ValueType,int64_t>;
            return;
          case DataType::UInt64BE:
            get_func = __getBE<ValueType,uint64_t>;
            put_func = __putBE<ValueType,uint64_t>;
            return;
          case DataType::Float32LE:
            get_func = __getLE<ValueType,float>;
            put_func = __putLE<ValueType,float>;
            return;
          case DataType::Float32BE:
            get_func = __getBE<ValueType,float>;
            put_func = __putBE<ValueType,float>;
            return;
          case DataType::Float64LE:
            get_func = __getLE<ValueType,double>;
            put_func = __putLE<ValueType,double>;
            return;
          case DataType::Float64BE:
            get_func = __getBE<ValueType,double>;
            put_func = __putBE<ValueType,double>;
            return;
          case DataType::CFloat32LE:
            get_func = __getLE<ValueType,cfloat>;
            put_func = __putLE<ValueType,cfloat>;
            return;
          case DataType::CFloat32BE:
            get_func = __getBE<ValueType,cfloat>;
            put_func = __putBE<ValueType,cfloat>;
            return;
          case DataType::CFloat64LE:
            get_func = __getLE<ValueType,cdouble>;
            put_func = __putLE<ValueType,cdouble>;
            return;
          case DataType::CFloat64BE:
            get_func = __getBE<ValueType,cdouble>;
            put_func = __putBE<ValueType,cdouble>;
            return;
          default:
            throw Exception ("invalid data type in image header");
        }
      }





    template <typename ValueType>
      Image<ValueType>::Buffer::Buffer (const Header& H, bool read_write_if_existing, bool direct_io, Stride::List strides) :
        Header (H) {
          if (H.is_file_backed()) { // file-backed image
            acquire_io (H);
            io->set_readwrite_if_existing (read_write_if_existing);
            io->open();
            set_get_put_functions (datatype());
          }
        }






    template <typename ValueType> 
      ValueType* Image<ValueType>::Buffer::get_data_pointer () const 
      {
        if (data_buffer) // already allocated via with_direct_io()
          return data_buffer.get();

        if (!io) { // scractch buffer: allocate and return
          data_buffer = std::unique_ptr<ValueType[]> (new ValueType [voxel_count (*this)]);
          return data_buffer.get();
        }

        // file-backed: check wether we can still do direct IO
        // if so, return address where mapped
        if (io->nsegments() == 1 && datatype() == DataType::from<ValueType>() && intensity_offset() == 0.0 && intensity_scale() == 1.0)
          return reinterpret_cast<ValueType*> (io->segment(0));
        
        // can't do direct IO
        return nullptr;
      }



    template <typename ValueType>
      inline const Image<ValueType> Header::get_image () const 
      {
        auto buffer = std::make_shared<const typename Image<ValueType>::Buffer> (*this);
        return { buffer };
      }






    template <typename ValueType>
      inline Image<ValueType>::Image (const std::shared_ptr<const Image<ValueType>::Buffer>& buffer_p, const Stride::List& strides) :
        buffer (buffer_p),
        data_pointer (buffer->get_data_pointer()),
        x (ndim(), 0),
        strides (strides.size() ? strides : Stride::get (*buffer)),
        data_offset (Stride::offset (*this))
        { 
          assert (data_pointer || buffer->get_io());
        }





    template <typename ValueType>
      inline Image<ValueType>::~Image () 
      {
        if (buffer.unique()) {
          // was image preloaded and read/write? If so,need to write back:
          if (buffer->get_io()) {
            if (buffer->get_io()->is_image_readwrite() && buffer->data_buffer) {
              auto data_buffer = std::move (buffer->data_buffer);
              TmpImage<ValueType> src = { *buffer, data_buffer.get(), std::vector<ssize_t> (ndim(), 0), strides, Stride::offset (*this) };
              Image<ValueType> dest (buffer);
              threaded_copy_with_progress_message ("writing back direct IO buffer for \"" + name() + "\"", src, dest); 
            }
          }
        }
      }



    template <typename ValueType>
      const Image<ValueType> Image<ValueType>::with_direct_io (Stride::List with_strides) const
      {
        if (!buffer->get_io())
          throw Exception ("FIXME: don't invoke 'with_direct_io()' on scratch images!");
        if (!buffer.unique())
          throw Exception ("FIXME: don't invoke 'with_direct_io()' on images if other copies exist!");

        bool preload = ( buffer->datatype() != DataType::from<ValueType>() );
        if (with_strides.size()) {
          auto new_strides = Stride::get_actual (Stride::get_nearest_match (*this, with_strides), *this);
          preload |= ( new_strides != Stride::get (*this) );
          with_strides = new_strides;
        }
        else 
          with_strides = Stride::get (*this); 

        if (!preload) 
          return std::move (*this);

        // do the preload:

        // the buffer into which to copy the data:
        buffer->data_buffer = std::unique_ptr<ValueType[]> (new ValueType [voxel_count (*this)]);

        if (buffer->get_io()->is_image_new()) {
          // no need to preload if data is zero anyway:
          memset (buffer->data_buffer.get(), 0, voxel_count (*this));
        }
        else {
          auto src (*this);
          TmpImage<ValueType> dest = { *buffer, buffer->data_buffer.get(), std::vector<ssize_t> (ndim(), 0), with_strides, Stride::offset (with_strides, *this) };
          threaded_copy_with_progress_message ("preloading data for \"" + name() + "\"", src, dest); 
        }

        return Image (buffer, with_strides);
      }




    //! \endcond

}

#endif


