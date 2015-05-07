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
        explicit Buffer (const std::string& image_name, bool readwrite = false) :
          ConstHeader (image_name) {
            assert (handler_);
            handler_->set_readwrite (readwrite);
            handler_->open();
            set_get_put_functions ();
          }

        //! construct a Buffer object to access the data in \a header
        explicit Buffer (const Header& header, bool readwrite = false) :
          ConstHeader (header) {
            handler_ = header.__get_handler();
            assert (handler_);
            handler_->set_readwrite (readwrite);
            handler_->open();
            set_get_put_functions ();
          }

        //! construct a Buffer object to access the same data as a buffer using a different data type.
        template <typename OtherValueType>
          explicit Buffer (const Buffer<OtherValueType>& buffer) :
            ConstHeader (buffer) {
              handler_ = buffer.__get_handler();
              assert (handler_);
              set_get_put_functions ();
            }

        //! construct a Buffer object to access the same data as another buffer.
        explicit Buffer (const Buffer<ValueType>& that) :
          ConstHeader (that) {
            handler_ = that.__get_handler();
            assert (handler_);
            set_get_put_functions ();
          }

        //! construct a Buffer object to create and access the image specified
        explicit Buffer (const std::string& image_name, const Header& template_header) :
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



