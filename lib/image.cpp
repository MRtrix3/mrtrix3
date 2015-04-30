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

#include "image.h"

namespace MR
{


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


  }





  template <typename ValueType>
    typename std::enable_if<is_data_type<ValueType>::value, void>::type __set_get_put_functions (
        std::function<ValueType(const void*,size_t,default_type,default_type)>& get_func,
        std::function<void(ValueType,void*,size_t,default_type,default_type)>& put_func, 
        DataType datatype) {

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

#undef MRTRIX_EXTERN
#define MRTRIX_EXTERN
  __DEFINE_SET_GET_PUT_FUNCTIONS;



}


