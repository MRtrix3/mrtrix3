/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "image_io/fetch_store.h"

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
      RAMType __fetch (const void* data, size_t i, default_type offset, default_type scale) {
        return round_func<RAMType> (scale_from_storage (Raw::fetch<DiskType> (data, i), offset, scale)); 
      }

    template <typename RAMType, typename DiskType> 
      void __store (RAMType val, void* data, size_t i, default_type offset, default_type scale) {
        return Raw::store<DiskType> (round_func<DiskType> (scale_to_storage (val, offset, scale)), data, i); 
      }

    // for little-endian multi-byte types:

    template <typename RAMType, typename DiskType> 
      RAMType __fetch_LE (const void* data, size_t i, default_type offset, default_type scale) {
        return round_func<RAMType> (scale_from_storage (Raw::fetch_LE<DiskType> (data, i), offset, scale));
      }

    template <typename RAMType, typename DiskType> 
      void __store_LE (RAMType val, void* data, size_t i, default_type offset, default_type scale) {
        return Raw::store_LE<DiskType> (round_func<DiskType> (scale_to_storage (val, offset, scale)), data, i);
      }


    // for big-endian multi-byte types:

    template <typename RAMType, typename DiskType> 
      RAMType __fetch_BE (const void* data, size_t i, default_type offset, default_type scale) {
        return round_func<RAMType> (scale_from_storage (Raw::fetch_BE<DiskType> (data, i), offset, scale));
      }

    template <typename RAMType, typename DiskType> 
      void __store_BE (RAMType val, void* data, size_t i, default_type offset, default_type scale) {
        return Raw::store_BE<DiskType> (round_func<DiskType> (scale_to_storage (val, offset, scale)), data, i);
      }


  }





  template <typename ValueType>
    typename std::enable_if<is_data_type<ValueType>::value, void>::type __set_fetch_store_functions (
        std::function<ValueType(const void*,size_t,default_type,default_type)>& fetch_func,
        std::function<void(ValueType,void*,size_t,default_type,default_type)>& store_func, 
        DataType datatype) {

      switch (datatype()) {
        case DataType::Bit:
          fetch_func = __fetch<ValueType,bool>;
          store_func = __store<ValueType,bool>;
          return;
        case DataType::Int8:
          fetch_func = __fetch<ValueType,int8_t>;
          store_func = __store<ValueType,int8_t>;
          return;
        case DataType::UInt8:
          fetch_func = __fetch<ValueType,uint8_t>;
          store_func = __store<ValueType,uint8_t>;
          return;
        case DataType::Int16LE:
          fetch_func = __fetch_LE<ValueType,int16_t>;
          store_func = __store_LE<ValueType,int16_t>;
          return;
        case DataType::UInt16LE:
          fetch_func = __fetch_LE<ValueType,uint16_t>;
          store_func = __store_LE<ValueType,uint16_t>;
          return;
        case DataType::Int16BE:
          fetch_func = __fetch_BE<ValueType,int16_t>;
          store_func = __store_BE<ValueType,int16_t>;
          return;
        case DataType::UInt16BE:
          fetch_func = __fetch_BE<ValueType,uint16_t>;
          store_func = __store_BE<ValueType,uint16_t>;
          return;
        case DataType::Int32LE:
          fetch_func = __fetch_LE<ValueType,int32_t>;
          store_func = __store_LE<ValueType,int32_t>;
          return;
        case DataType::UInt32LE:
          fetch_func = __fetch_LE<ValueType,uint32_t>;
          store_func = __store_LE<ValueType,uint32_t>;
          return;
        case DataType::Int32BE:
          fetch_func = __fetch_BE<ValueType,int32_t>;
          store_func = __store_BE<ValueType,int32_t>;
          return;
        case DataType::UInt32BE:
          fetch_func = __fetch_BE<ValueType,uint32_t>;
          store_func = __store_BE<ValueType,uint32_t>;
          return;
        case DataType::Int64LE:
          fetch_func = __fetch_LE<ValueType,int64_t>;
          store_func = __store_LE<ValueType,int64_t>;
          return;
        case DataType::UInt64LE:
          fetch_func = __fetch_LE<ValueType,uint64_t>;
          store_func = __store_LE<ValueType,uint64_t>;
          return;
        case DataType::Int64BE:
          fetch_func = __fetch_BE<ValueType,int64_t>;
          store_func = __store_BE<ValueType,int64_t>;
          return;
        case DataType::UInt64BE:
          fetch_func = __fetch_BE<ValueType,uint64_t>;
          store_func = __store_BE<ValueType,uint64_t>;
          return;
        case DataType::Float32LE:
          fetch_func = __fetch_LE<ValueType,float>;
          store_func = __store_LE<ValueType,float>;
          return;
        case DataType::Float32BE:
          fetch_func = __fetch_BE<ValueType,float>;
          store_func = __store_BE<ValueType,float>;
          return;
        case DataType::Float64LE:
          fetch_func = __fetch_LE<ValueType,double>;
          store_func = __store_LE<ValueType,double>;
          return;
        case DataType::Float64BE:
          fetch_func = __fetch_BE<ValueType,double>;
          store_func = __store_BE<ValueType,double>;
          return;
        case DataType::CFloat32LE:
          fetch_func = __fetch_LE<ValueType,cfloat>;
          store_func = __store_LE<ValueType,cfloat>;
          return;
        case DataType::CFloat32BE:
          fetch_func = __fetch_BE<ValueType,cfloat>;
          store_func = __store_BE<ValueType,cfloat>;
          return;
        case DataType::CFloat64LE:
          fetch_func = __fetch_LE<ValueType,cdouble>;
          store_func = __store_LE<ValueType,cdouble>;
          return;
        case DataType::CFloat64BE:
          fetch_func = __fetch_BE<ValueType,cdouble>;
          store_func = __store_BE<ValueType,cdouble>;
          return;
        default:
          throw Exception ("invalid data type in image header");
      }
    }

#undef MRTRIX_EXTERN
#define MRTRIX_EXTERN
  __DEFINE_FETCH_STORE_FUNCTIONS;

}


