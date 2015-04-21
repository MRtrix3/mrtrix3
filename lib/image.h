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
//#include "copy.h"
//#include "threaded_copy.h"

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

        //! used internally to instantiate Image objects
        Image (Buffer*);

        const Header& header () const { return *buffer; }

        const std::string& name() const { return buffer->name(); }
        const Math::Matrix<default_type>& transform() const { return buffer->transform(); }

        size_t  ndim () const { return buffer->ndim(); }
        ssize_t size (size_t axis) const { return buffer->size (axis); }
        default_type voxsize (size_t axis) const { return buffer->voxsize (axis); }
        ssize_t stride (size_t axis) const { return buffer->stride (axis); }
        DataType datatype () const { return buffer->datatype(); }

        //! offset to current voxel from start of data
        size_t offset () const { return data_offset; }

        //! get position of current voxel location along \a axis
        ssize_t index (size_t axis) const { return get_voxel_position (axis); }
        //! get/set position of current voxel location along \a axis
        Helper::VoxelIndex<Image> index (size_t axis) { return Helper::VoxelIndex<Image> (*this, axis); }

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

        friend class Helper::VoxelIndex<Image>;
        friend class Helper::VoxelValue<Image>;
    };








    template <typename ValueType> 
      class Image<ValueType>::Buffer : public Header
    {
      public:
        //! construct a Buffer object to access the data in the image specified
        Buffer (const Header& H, bool read_write_if_existing = false, bool preload = false, Stride::List strides = Stride::List());
        Buffer (Buffer&&) = default;
        Buffer (const Buffer&) = delete;
        Buffer& operator= (const Buffer&) = delete;

        ValueType get_value (size_t offset) const {
          ssize_t nseg = offset / io->segment_size();
          return get_func (io->segment (nseg), offset - nseg*io->segment_size(), intensity_offset(), intensity_scale());
        }

        void set_value (size_t offset, ValueType val) const {
          ssize_t nseg = offset / io->segment_size();
          put_func (val, io->segment (nseg), offset - nseg*io->segment_size(), intensity_offset(), intensity_scale());
        }

        std::unique_ptr<ValueType[]> data_buffer;
        ValueType* get_data_pointer () const;

      protected:
        std::function<ValueType(const void*,size_t,default_type,default_type)> get_func;
        std::function<void(ValueType,void*,size_t,default_type,default_type)> put_func;

        void set_get_put_functions (DataType datatype);
    };











    //! \cond skip
    namespace {

      template <class... DestImageType>
        struct __assign {
          __assign (size_t axis, ssize_t index) : axis (axis), index (index) { }
          const size_t axis;
          const ssize_t index;
          template <class ImageType> 
            void operator() (ImageType& x) { x.index (axis) = index; }
        };

      template <class... DestImageType>
        struct __assign<std::tuple<DestImageType...>> {
          __assign (size_t axis, ssize_t index) : axis (axis), index (index) { }
          const size_t axis;
          const ssize_t index;
          template <class ImageType> 
            void operator() (ImageType& x) { std::get<0>(x).index (axis) = index; }
        };

      template <class... DestImageType>
        struct __max_axis {
          __max_axis (size_t& axis) : axis (axis) { }
          size_t& axis;
          template <class ImageType> 
            void operator() (ImageType& x) { if (axis > x.ndim()) axis = x.ndim(); }
        };

      template <class... DestImageType>
        struct __max_axis<std::tuple<DestImageType...>> {
          __max_axis (size_t& axis) : axis (axis) { }
          size_t& axis;
          template <class ImageType> 
            void operator() (ImageType& x) { if (axis > std::get<0>(x).ndim()) axis = std::get<0>(x).ndim(); }
        };

      template <class ImageType>
        struct __assign_pos_axis_range
        {
          template <class... DestImageType>
            void to (DestImageType&... dest) const {
              apply (__max_axis<DestImageType...> (to_axis), std::tie (dest...));
              for (size_t n = from_axis; n < to_axis; ++n)
                apply (__assign<DestImageType...> (n, ref.index(n)), std::tie (dest...));
            }
          const ImageType& ref;
          const size_t from_axis;
          size_t to_axis;
        };


      template <class ImageType, typename IntType>
        struct __assign_pos_axes
        {
          template <class... DestImageType>
            void to (DestImageType&... dest) const {
              for (auto a : axes) 
                apply (__assign<DestImageType...> (a, ref.index(a)), std::tie (dest...));
            }
          const ImageType& ref;
          const std::vector<IntType> axes;
        };
    }

    //! \endcond


    //! returns a functor to set the position in ref to other voxels
    /*! this can be used as follows:
     * \code
     * assign_pos_of (src_image, 0, 3).to (dest_image1, dest_image2);
     * \endcode */
    template <class ImageType>
      inline __assign_pos_axis_range<ImageType> 
      assign_pos_of (const ImageType& reference, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) 
      {
        return { reference, from_axis, to_axis };
      }

    //! returns a functor to set the position in ref to other voxels
    /*! this can be used as follows:
     * \code
     * std::vector<int> axes = { 0, 3, 4 };
     * assign_pos (src_image, axes) (dest_image1, dest_image2);
     * \endcode */
    template <class ImageType, typename IntType>
      inline __assign_pos_axes<ImageType, IntType> 
      assign_pos_of (const ImageType& reference, const std::vector<IntType>& axes) 
      {
        return { reference, axes };
      }

    //! \copydoc __assign_pos_axes
    template <class ImageType, typename IntType>
      inline __assign_pos_axes<ImageType, IntType> 
      assign_pos_of (const ImageType& reference, const std::vector<IntType>&& axes) 
      {
        return assign_pos_of (reference, axes);
      }








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



      // for single-byte types:

      template <typename RAMType, typename DiskType> 
        RAMType __get (const void* data, size_t i, default_type offset, default_type scale) {
          return round_func<RAMType> (scale_from_storage (MR::get<DiskType> (data, i), offset, scale)); 
        }

      template <typename RAMType, typename DiskType> 
        void __put (RAMType val, void* data, size_t i, default_type offset, default_type scale) {
          return MR::put<DiskType> (round_func<DiskType> ((val - offset) / scale), data, i); 
        }

      // for little-endian multi-byte types:

      template <typename RAMType, typename DiskType> 
        RAMType __getLE (const void* data, size_t i, default_type offset, default_type scale) {
          return round_func<RAMType> (scale_from_storage (MR::getLE<DiskType> (data, i), offset, scale)); 
        }

      template <typename RAMType, typename DiskType> 
        void __putLE (RAMType val, void* data, size_t i, default_type offset, default_type scale) {
          return MR::putLE<DiskType> (round_func<DiskType> ((val - offset) / scale), data, i); 
        }


      // for big-endian multi-byte types:

      template <typename RAMType, typename DiskType> 
        RAMType __getBE (const void* data, size_t i, default_type offset, default_type scale) {
          return round_func<RAMType> (scale_from_storage (MR::getBE<DiskType> (data, i), offset, scale)); 
        }

      template <typename RAMType, typename DiskType> 
        void __putBE (RAMType val, void* data, size_t i, default_type offset, default_type scale) {
          return MR::putBE<DiskType> (round_func<DiskType> ((val - offset) / scale), data, i); 
        }


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
      Image<ValueType>::Buffer::Buffer (const Header& H, bool read_write_if_existing, bool preload, Stride::List strides) :
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
        if (data_buffer)
          return data_buffer.get();

        assert (io);
        if (io->nsegments() == 1 && datatype() == DataType::from<ValueType>() && intensity_offset() == 0.0 && intensity_scale() == 1.0)
          return reinterpret_cast<ValueType*> (io->segment(0));
        else 
          return nullptr;
      }



    template <typename ValueType>
      inline Image<ValueType> Header::get_image () const 
      {
        return Image<ValueType> (new typename Image<ValueType>::Buffer (*this)); 
      }






    template <typename ValueType>
      inline Image<ValueType>::Image (Image<ValueType>::Buffer* buffer_p) :
        buffer (buffer_p),
        data_pointer (buffer->get_data_pointer()),
        x (ndim(), 0),
        data_offset (Stride::offset (*buffer))
      {

      }

    
    //! \endcond

}

#endif


