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
      template <typename value_type, typename S> 
        value_type __get (const void* data, size_t i) 
        {
          return value_type (MR::get<S> (data, i)); 
        }

      template <typename value_type, typename S> 
        value_type __getLE (const void* data, size_t i) 
        { 
          return value_type (MR::getLE<S> (data, i)); 
        }

      template <typename value_type, typename S> 
        value_type __getBE (const void* data, size_t i) 
        {
          return value_type (MR::getBE<S> (data, i)); 
        }


      template <typename value_type, typename S> 
        void __put (value_type val, void* data, size_t i) 
        {
          return MR::put<S> (S (val), data, i); 
        }

      template <typename value_type, typename S> 
        void __putLE (value_type val, void* data, size_t i) 
        {
          return MR::putLE<S> (S (val), data, i);
        }

      template <typename value_type, typename S> 
        void __putBE (value_type val, void* data, size_t i) 
        {
          return MR::putBE<S> (S (val), data, i); 
        }



      // needed to round floating-point values and map non-finite values (NaN, Inf) to zero for integer types:
      template <typename value_out_type, typename value_type> 
        inline value_out_type round_finite (value_type val) 
        { 
          return std::isfinite (val) ? 
            value_out_type(Math::round (val)) : 
            value_out_type (0); 
        }

      template <typename value_out_type, typename value_type> 
        inline value_out_type no_round (value_type val) 
        {
          return val; 
        }

      // specialisations for conversion between real types and complex types, and integer types and floating-point types:

#define GET_FUNC_BO_REAL(type,round_func) \
      template <> type __getLE<type,float> (const void* data, size_t i) { return round_func<type> (MR::getLE<float>(data, i)); } \
      template <> type __getBE<type,float> (const void* data, size_t i) { return round_func<type> (MR::getBE<float>(data, i)); } \
      template <> type __getLE<type,double> (const void* data, size_t i) { return round_func<type> (MR::getLE<double>(data, i)); } \
      template <> type __getBE<type,double> (const void* data, size_t i) { return round_func<type> (MR::getBE<double>(data, i)); } 

#define GET_PUT_FUNC_REAL(type,round_func) \
      GET_FUNC_BO_REAL(type,round_func) \
      template <> void __put<float,type> (float val, void* data, size_t i) { return MR::put<type> (round_func<type> (val), data, i); } \
      template <> void __put<double,type> (double val, void* data, size_t i) { return MR::put<type> (round_func<type> (val), data, i); }

#define GET_PUT_FUNC_BO_REAL(type,round_func) \
      GET_FUNC_BO_REAL(type,round_func) \
      template <> void __putLE<float,type> (float val, void* data, size_t i) { return MR::putLE<type> (round_func<type> (val), data, i); } \
      template <> void __putBE<float,type> (float val, void* data, size_t i) { return MR::putBE<type> (round_func<type> (val), data, i); } \
      template <> void __putLE<double,type> (double val, void* data, size_t i) { return MR::putLE<type> (round_func<type> (val), data, i); } \
      template <> void __putBE<double,type> (double val, void* data, size_t i) { return MR::putBE<type> (round_func<type> (val), data, i); } 

#define GET_FUNC_BO_COMPLEX(type,round_func) \
      template <> type __getLE<type,cfloat> (const void* data, size_t i) { return round_func<type> (MR::getLE<cfloat>(data, i).real()); } \
      template <> type __getBE<type,cfloat> (const void* data, size_t i) { return round_func<type> (MR::getBE<cfloat>(data, i).real()); } \
      template <> type __getLE<type,cdouble> (const void* data, size_t i) { return round_func<type> (MR::getLE<cdouble>(data, i).real()); } \
      template <> type __getBE<type,cdouble> (const void* data, size_t i) { return round_func<type> (MR::getBE<cdouble>(data, i).real()); } 

#define GET_PUT_FUNC_COMPLEX(type,round_func) \
      GET_FUNC_BO_COMPLEX(type,round_func) \
      template <> void __put<cfloat,type> (cfloat val, void* data, size_t i) { return MR::put<type> (round_func<type> (val.real()), data, i); } \
      template <> void __put<cdouble,type> (cdouble val, void* data, size_t i) { return MR::put<type> (round_func<type> (val.real()), data, i); }

#define GET_PUT_FUNC_BO_COMPLEX(type,round_func) \
      GET_FUNC_BO_COMPLEX(type,round_func) \
      template <> void __putLE<cfloat,type> (cfloat val, void* data, size_t i) { return MR::putLE<type> (round_func<type> (val.real()), data, i); } \
      template <> void __putBE<cfloat,type> (cfloat val, void* data, size_t i) { return MR::putBE<type> (round_func<type> (val.real()), data, i); } \
      template <> void __putLE<cdouble,type> (cdouble val, void* data, size_t i) { return MR::putLE<type> (round_func<type> (val.real()), data, i); } \
      template <> void __putBE<cdouble,type> (cdouble val, void* data, size_t i) { return MR::putBE<type> (round_func<type> (val.real()), data, i); }

#define GET_PUT_FUNC(type,round_func) \
      GET_PUT_FUNC_REAL(type,round_func); \
      GET_PUT_FUNC_COMPLEX(type,round_func); 

#define GET_PUT_FUNC_BO(type,round_func) \
      GET_PUT_FUNC_BO_REAL(type,round_func); \
      GET_PUT_FUNC_BO_COMPLEX(type,round_func); 


      // conversion to/from integer types that don't require byte-swapping:
      GET_PUT_FUNC(bool,round_finite);
      GET_PUT_FUNC(int8_t,round_finite);
      GET_PUT_FUNC(uint8_t,round_finite);

      // conversion to/from integer types that do require byte-swapping:
      GET_PUT_FUNC_BO(int16_t,round_finite);
      GET_PUT_FUNC_BO(uint16_t,round_finite);
      GET_PUT_FUNC_BO(int32_t,round_finite);
      GET_PUT_FUNC_BO(uint32_t,round_finite);
      GET_PUT_FUNC_BO(int64_t,round_finite);
      GET_PUT_FUNC_BO(uint64_t,round_finite);

      // conversion between non-integer real & complex types:
      GET_PUT_FUNC_BO_COMPLEX(float32,no_round);
      GET_PUT_FUNC_BO_COMPLEX(float64,no_round);
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
        template <class Set> 
          Buffer& operator= (const Set& H) { assert (0); return *this; }

        value_type (*get_func) (const void* data, size_t i);
        void (*put_func) (value_type val, void* data, size_t i);

        void set_get_put_functions () {
          switch (datatype() ()) {
            case DataType::Bit:
              get_func = &__get<value_type,bool>;
              put_func = &__put<value_type,bool>;
              return;
            case DataType::Int8:
              get_func = &__get<value_type,int8_t>;
              put_func = &__put<value_type,int8_t>;
              return;
            case DataType::UInt8:
              get_func = &__get<value_type,uint8_t>;
              put_func = &__put<value_type,uint8_t>;
              return;
            case DataType::Int16LE:
              get_func = &__getLE<value_type,int16_t>;
              put_func = &__putLE<value_type,int16_t>;
              return;
            case DataType::UInt16LE:
              get_func = &__getLE<value_type,uint16_t>;
              put_func = &__putLE<value_type,uint16_t>;
              return;
            case DataType::Int16BE:
              get_func = &__getBE<value_type,int16_t>;
              put_func = &__putBE<value_type,int16_t>;
              return;
            case DataType::UInt16BE:
              get_func = &__getBE<value_type,uint16_t>;
              put_func = &__putBE<value_type,uint16_t>;
              return;
            case DataType::Int32LE:
              get_func = &__getLE<value_type,int32_t>;
              put_func = &__putLE<value_type,int32_t>;
              return;
            case DataType::UInt32LE:
              get_func = &__getLE<value_type,uint32_t>;
              put_func = &__putLE<value_type,uint32_t>;
              return;
            case DataType::Int32BE:
              get_func = &__getBE<value_type,int32_t>;
              put_func = &__putBE<value_type,int32_t>;
              return;
            case DataType::UInt32BE:
              get_func = &__getBE<value_type,uint32_t>;
              put_func = &__putBE<value_type,uint32_t>;
              return;
            case DataType::Int64LE:
              get_func = &__getLE<value_type,int64_t>;
              put_func = &__putLE<value_type,int64_t>;
              return;
            case DataType::UInt64LE:
              get_func = &__getLE<value_type,uint64_t>;
              put_func = &__putLE<value_type,uint64_t>;
              return;
            case DataType::Int64BE:
              get_func = &__getBE<value_type,int64_t>;
              put_func = &__putBE<value_type,int64_t>;
              return;
            case DataType::UInt64BE:
              get_func = &__getBE<value_type,uint64_t>;
              put_func = &__putBE<value_type,uint64_t>;
              return;
            case DataType::Float32LE:
              get_func = &__getLE<value_type,float>;
              put_func = &__putLE<value_type,float>;
              return;
            case DataType::Float32BE:
              get_func = &__getBE<value_type,float>;
              put_func = &__putBE<value_type,float>;
              return;
            case DataType::Float64LE:
              get_func = &__getLE<value_type,double>;
              put_func = &__putLE<value_type,double>;
              return;
            case DataType::Float64BE:
              get_func = &__getBE<value_type,double>;
              put_func = &__putBE<value_type,double>;
              return;
            case DataType::CFloat32LE:
              get_func = &__getLE<value_type,cfloat>;
              put_func = &__putLE<value_type,cfloat>;
              return;
            case DataType::CFloat32BE:
              get_func = &__getBE<value_type,cfloat>;
              put_func = &__putBE<value_type,cfloat>;
              return;
            case DataType::CFloat64LE:
              get_func = &__getLE<value_type,cdouble>;
              put_func = &__putLE<value_type,cdouble>;
              return;
            case DataType::CFloat64BE:
              get_func = &__getBE<value_type,cdouble>;
              put_func = &__putBE<value_type,cdouble>;
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



