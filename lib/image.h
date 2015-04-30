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
#include "header.h"
#include "get_set.h"
#include "image_helpers.h"
#include "algo/copy.h"
#include "algo/threaded_copy.h"

namespace MR
{
  // TODO make sure buffer are allocated to correct size for bool
  // will probably require using uint8_t as pointer type...
  namespace {

    template <typename ValueType>
      inline ValueType __get_value_direct_io (const ValueType* data, size_t offset) {
        return data[offset];
      }
    template <>
      inline bool __get_value_direct_io (const bool* data, size_t offset) {
        return get<bool> (data, offset);
      }

    template <typename ValueType>
      inline void __set_value_direct_io (ValueType val, ValueType* data, size_t offset) {
        data[offset] = val;
      }
    template <>
      inline void __set_value_direct_io (bool val, bool* data, size_t offset) {
        put<bool> (val, reinterpret_cast<void*>(data), offset, true);
      }

  }


  template <typename ValueType>
    class Image {
      public:
        typedef ValueType value_type;
        class Buffer;

        Image ();
        Image (const Image&) = default;
        Image (Image&&) = default;
        Image& operator= (const Image& image) = default;
        Image& operator= (Image&&) = default;
        ~Image();

        //! used internally to instantiate Image objects
        Image (const std::shared_ptr<Buffer>&, const Stride::List& = Stride::List());

        bool valid () const { return bool(buffer); }
        bool operator! () const { return !valid(); }

        const Header& header () const { return *buffer; }

        const std::string& name() const { return buffer->name(); }
        const Math::Matrix<default_type>& transform() const { return buffer->transform(); }

        size_t  ndim () const { return buffer->ndim(); }
        ssize_t size (size_t axis) const { return buffer->size (axis); }
        default_type voxsize (size_t axis) const { return buffer->voxsize (axis); }
        ssize_t stride (size_t axis) const { return strides[axis]; }

        //! offset to current voxel from start of data
        size_t offset () const { return data_offset; }

        //! reset index to zero (origin)
        void reset () {
          for (size_t n = 0; n < ndim(); ++n)
            index(n) = 0;
        }

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
        Image with_direct_io (Stride::List with_strides = Stride::List());


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
        std::shared_ptr<Buffer> buffer;
        //! pointer to data address whether in RAM or MMap
        ValueType* data_pointer;
        //! voxel indices
        std::vector<ssize_t> x;
        //! voxel indices
        Stride::List strides;
        //! offset to currently pointed-to voxel
        size_t data_offset;

        ValueType get_voxel_value () const {
          if (data_pointer) return __get_value_direct_io (data_pointer, data_offset);
          return buffer->get_value (data_offset);
        }

        void set_voxel_value (ValueType val) {
          if (data_pointer) __set_value_direct_io (val, data_pointer, data_offset);
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



        friend class Helper::VoxelIndex<Image>;
        friend class Helper::VoxelValue<Image>;
    };







  template <typename ValueType> 
    class Image<ValueType>::Buffer : public Header
    {
      public:
        //! construct a Buffer object to access the data in the image specified
        Buffer (Header& H, bool read_write_if_existing = false, bool direct_io = false, Stride::List strides = Stride::List());
        Buffer (Buffer&&) = default;
        Buffer& operator= (const Buffer&) = delete;
        Buffer& operator= (Buffer&&) = default;
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

        std::unique_ptr<ValueType[]> data_buffer;
        ValueType* get_data_pointer ();

        ImageIO::Base* get_io () const { return io.get(); }

      protected:
        std::function<ValueType(const void*,size_t,default_type,default_type)> get_func;
        std::function<void(ValueType,void*,size_t,default_type,default_type)> put_func;

        void set_get_put_functions ();
    };













  //! \cond skip

  namespace
  {

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
        ssize_t index (size_t axis) const { return get_voxel_position (axis); }
        Helper::VoxelIndex<TmpImage> index (size_t axis) { return { *this, axis }; }
        value_type value () const { return get_voxel_value(); }
        Helper::VoxelValue<TmpImage> value () { return { *this }; }

        void set_voxel_value (ValueType val) { __set_value_direct_io (val, data, offset); }
        value_type get_voxel_value () const { return __get_value_direct_io (data, offset); }
        ssize_t get_voxel_position (size_t axis) const { return x[axis]; }
        void set_voxel_position (size_t axis, ssize_t position) { offset += stride (axis) * (position - x[axis]); x[axis] = position; }
        void move_voxel_position (size_t axis, ssize_t increment) { offset += stride (axis) * increment; x[axis] += increment; }
      };

  }



  template <typename ValueType>
    typename std::enable_if<!is_data_type<ValueType>::value, void>::type __set_get_put_functions (
        std::function<ValueType(const void*,size_t,default_type,default_type)>& get_func,
        std::function<void(ValueType,void*,size_t,default_type,default_type)>& put_func, 
        DataType datatype) { }



  template <typename ValueType>
    typename std::enable_if<is_data_type<ValueType>::value, void>::type __set_get_put_functions (
        std::function<ValueType(const void*,size_t,default_type,default_type)>& get_func,
        std::function<void(ValueType,void*,size_t,default_type,default_type)>& put_func, 
        DataType datatype);

  template <typename ValueType> 
    inline void Image<ValueType>::Buffer::set_get_put_functions () {
      __set_get_put_functions (get_func, put_func, datatype());
    }






  template <typename ValueType>
    Image<ValueType>::Buffer::Buffer (Header& H, bool read_write_if_existing, bool direct_io, Stride::List strides) :
      Header (H) {
        assert (H.valid()); // IO handler set
        assert (H.is_file_backed() ? is_data_type<ValueType>::value : true);

        acquire_io (H);
        io->set_readwrite_if_existing (read_write_if_existing);
        io->open (*this, sizeof(ValueType));
        if (io->is_file_backed()) 
          set_get_put_functions ();
      }






  template <typename ValueType> 
    ValueType* Image<ValueType>::Buffer::get_data_pointer () 
    {
      if (data_buffer) // already allocated via with_direct_io()
        return data_buffer.get();

      assert (io);
      if (!io->is_file_backed()) // this is a scratch image
        return reinterpret_cast<ValueType*> (io->segment(0));

      // check wether we can still do direct IO
      // if so, return address where mapped
      if (io->nsegments() == 1 && datatype() == DataType::from<ValueType>() && intensity_offset() == 0.0 && intensity_scale() == 1.0)
        return reinterpret_cast<ValueType*> (io->segment(0));

      // can't do direct IO
      return nullptr;
    }



  template <typename ValueType>
    Image<ValueType> Header::get_image () 
    {
      assert (valid());
      if (!valid())
        throw Exception ("FIXME: don't invoke get_image() with invalid Header!");
      auto buffer = std::make_shared<typename Image<ValueType>::Buffer> (*this);
      return { buffer };
    }






  template <typename ValueType>
    inline Image<ValueType>::Image () :
      data_pointer (nullptr), 
      data_offset (0)
  { }

  template <typename ValueType>
    Image<ValueType>::Image (const std::shared_ptr<Image<ValueType>::Buffer>& buffer_p, const Stride::List& desired_strides) :
      buffer (buffer_p),
      data_pointer (buffer->get_data_pointer()),
      x (ndim(), 0),
      strides (desired_strides.size() ? desired_strides : Stride::get (*buffer)),
      data_offset (Stride::offset (*this))
      { 
        assert (buffer);
        assert (data_pointer || buffer->get_io());
        DEBUG ("image \"" + name() + "\" initialised with strides = " + str(strides) + ", start = " + str(data_offset));
      }





  template <typename ValueType>
    Image<ValueType>::~Image () 
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
    Image<ValueType> Image<ValueType>::with_direct_io (Stride::List with_strides)
    {
      if (buffer->data_buffer)
        throw Exception ("FIXME: don't invoke 'with_direct_io()' on images already using direct IO!");
      if (!buffer->get_io())
        throw Exception ("FIXME: don't invoke 'with_direct_io()' on invalid images!");
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



  //define get/put methods for all types using C++11 extern templates, 
  //to avoid massive compile times...
#define __DEFINE_SET_GET_PUT_FUNCTION_FOR_TYPE(ValueType) \
  MRTRIX_EXTERN template void __set_get_put_functions<ValueType> ( \
      std::function<ValueType(const void*,size_t,default_type,default_type)>& get_func, \
        std::function<void(ValueType,void*,size_t,default_type,default_type)>& put_func, \
        DataType datatype); \
  MRTRIX_EXTERN template Image<ValueType>::Buffer::Buffer (Header& H, bool read_write_if_existing, bool direct_io, Stride::List strides); \
  MRTRIX_EXTERN template ValueType* Image<ValueType>::Buffer::get_data_pointer (); \
  MRTRIX_EXTERN template Image<ValueType> Header::get_image (); \
  MRTRIX_EXTERN template Image<ValueType>::Image (const std::shared_ptr<Image<ValueType>::Buffer>& buffer_p, const Stride::List& desired_strides); \
  MRTRIX_EXTERN template Image<ValueType>::~Image (); \
  MRTRIX_EXTERN template Image<ValueType> Image<ValueType>::with_direct_io (Stride::List with_strides)

#define __DEFINE_SET_GET_PUT_FUNCTIONS \
  __DEFINE_SET_GET_PUT_FUNCTION_FOR_TYPE(bool); \
  __DEFINE_SET_GET_PUT_FUNCTION_FOR_TYPE(uint8_t); \
  __DEFINE_SET_GET_PUT_FUNCTION_FOR_TYPE(int8_t); \
  __DEFINE_SET_GET_PUT_FUNCTION_FOR_TYPE(uint16_t); \
  __DEFINE_SET_GET_PUT_FUNCTION_FOR_TYPE(int16_t); \
  __DEFINE_SET_GET_PUT_FUNCTION_FOR_TYPE(uint32_t); \
  __DEFINE_SET_GET_PUT_FUNCTION_FOR_TYPE(int32_t); \
  __DEFINE_SET_GET_PUT_FUNCTION_FOR_TYPE(uint64_t); \
  __DEFINE_SET_GET_PUT_FUNCTION_FOR_TYPE(int64_t); \
  __DEFINE_SET_GET_PUT_FUNCTION_FOR_TYPE(float); \
  __DEFINE_SET_GET_PUT_FUNCTION_FOR_TYPE(double); \
  __DEFINE_SET_GET_PUT_FUNCTION_FOR_TYPE(cfloat); \
  __DEFINE_SET_GET_PUT_FUNCTION_FOR_TYPE(cdouble); \

#define MRTRIX_EXTERN extern
  __DEFINE_SET_GET_PUT_FUNCTIONS;



  //! \endcond

}

#endif


