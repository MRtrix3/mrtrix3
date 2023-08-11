/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "fetch_store.h"
#include "types.h"



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
        return TypeOUT(in);
      }

    // integer -> integer
    template <typename TypeOUT, typename TypeIN>
      inline typename std::enable_if<std::is_integral<TypeOUT>::value, TypeOUT>::type
      round_func (TypeIN in, typename std::enable_if<std::is_integral<TypeIN>::value>::type* = nullptr) {
        return TypeOUT(in);
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
      RAMType __fetch (const void* data, size_t i) {
        return round_func<RAMType> (Raw::fetch<DiskType> (data, i));
      }

    template <typename RAMType, typename DiskType>
      void __store (RAMType val, void* data, size_t i) {
        return Raw::store<DiskType> (round_func<DiskType> (val), data, i);
      }

    template <typename RAMType, typename DiskType>
      RAMType __fetch_scale (const void* data, size_t i, default_type offset, default_type scale) {
        return round_func<RAMType> (scale_from_storage (Raw::fetch<DiskType> (data, i), offset, scale));
      }

    template <typename RAMType, typename DiskType>
      void __scale_store (RAMType val, void* data, size_t i, default_type offset, default_type scale) {
        return Raw::store<DiskType> (round_func<DiskType> (scale_to_storage (val, offset, scale)), data, i);
      }

    // for little-endian multi-byte types:

    template <typename RAMType, typename DiskType>
      RAMType __fetch_LE (const void* data, size_t i) {
        return round_func<RAMType> (Raw::fetch_LE<DiskType> (data, i));
      }

    template <typename RAMType, typename DiskType>
      void __store_LE (RAMType val, void* data, size_t i) {
        return Raw::store_LE<DiskType> (round_func<DiskType> (val), data, i);
      }

    template <typename RAMType, typename DiskType>
      RAMType __fetch_scale_LE (const void* data, size_t i, default_type offset, default_type scale) {
        return round_func<RAMType> (scale_from_storage (Raw::fetch_LE<DiskType> (data, i), offset, scale));
      }

    template <typename RAMType, typename DiskType>
      void __scale_store_LE (RAMType val, void* data, size_t i, default_type offset, default_type scale) {
        return Raw::store_LE<DiskType> (round_func<DiskType> (scale_to_storage (val, offset, scale)), data, i);
      }


    // for big-endian multi-byte types:

    template <typename RAMType, typename DiskType>
      RAMType __fetch_BE (const void* data, size_t i) {
        return round_func<RAMType> (Raw::fetch_BE<DiskType> (data, i));
      }

    template <typename RAMType, typename DiskType>
      void __store_BE (RAMType val, void* data, size_t i) {
        return Raw::store_BE<DiskType> (round_func<DiskType> (val), data, i);
      }

    template <typename RAMType, typename DiskType>
      RAMType __fetch_scale_BE (const void* data, size_t i, default_type offset, default_type scale) {
        return round_func<RAMType> (scale_from_storage (Raw::fetch_BE<DiskType> (data, i), offset, scale));
      }

    template <typename RAMType, typename DiskType>
      void __scale_store_BE (RAMType val, void* data, size_t i, default_type offset, default_type scale) {
        return Raw::store_BE<DiskType> (round_func<DiskType> (scale_to_storage (val, offset, scale)), data, i);
      }


  }




  template <typename ValueType>
    typename std::enable_if<is_data_type<ValueType>::value, std::function<ValueType(const void*,size_t)>>::type __set_fetch_function (const DataType datatype)
  {
    switch (datatype()) {
      case DataType::Bit:
        return __fetch<ValueType,bool>;
      case DataType::Int8:
        return __fetch<ValueType,int8_t>;
      case DataType::UInt8:
        return __fetch<ValueType,uint8_t>;
      case DataType::Int16LE:
        return __fetch_LE<ValueType,int16_t>;
      case DataType::UInt16LE:
        return __fetch_LE<ValueType,uint16_t>;
      case DataType::Int16BE:
        return __fetch_BE<ValueType,int16_t>;
      case DataType::UInt16BE:
        return __fetch_BE<ValueType,uint16_t>;
      case DataType::Int32LE:
        return __fetch_LE<ValueType,int32_t>;
      case DataType::UInt32LE:
        return __fetch_LE<ValueType,uint32_t>;
      case DataType::Int32BE:
        return __fetch_BE<ValueType,int32_t>;
      case DataType::UInt32BE:
        return __fetch_BE<ValueType,uint32_t>;
      case DataType::Int64LE:
        return __fetch_LE<ValueType,int64_t>;
      case DataType::UInt64LE:
        return __fetch_LE<ValueType,uint64_t>;
      case DataType::Int64BE:
        return __fetch_BE<ValueType,int64_t>;
      case DataType::UInt64BE:
        return __fetch_BE<ValueType,uint64_t>;
      case DataType::Float16LE:
        return __fetch_LE<ValueType,half_float::half>;
      case DataType::Float16BE:
        return __fetch_BE<ValueType,half_float::half>;
      case DataType::Float32LE:
        return __fetch_LE<ValueType,float>;
      case DataType::Float32BE:
        return __fetch_BE<ValueType,float>;
      case DataType::Float64LE:
        return __fetch_LE<ValueType,double>;
      case DataType::Float64BE:
        return __fetch_BE<ValueType,double>;
      case DataType::CFloat32LE:
        return __fetch_LE<ValueType,cfloat>;
      case DataType::CFloat32BE:
        return __fetch_BE<ValueType,cfloat>;
      case DataType::CFloat64LE:
        return __fetch_LE<ValueType,cdouble>;
      case DataType::CFloat64BE:
        return __fetch_BE<ValueType,cdouble>;
      default:
        throw Exception ("Unknown data type in request for fetch function");
    }
  }



  template <typename ValueType>
    typename std::enable_if<is_data_type<ValueType>::value, std::function<void(ValueType,void*,size_t)>>::type __set_store_function (const DataType datatype)
  {
    switch (datatype()) {
      case DataType::Bit:
        return __store<ValueType,bool>;
      case DataType::Int8:
        return __store<ValueType,int8_t>;
      case DataType::UInt8:
        return __store<ValueType,uint8_t>;
      case DataType::Int16LE:
        return __store_LE<ValueType,int16_t>;
      case DataType::UInt16LE:
        return __store_LE<ValueType,uint16_t>;
      case DataType::Int16BE:
        return __store_BE<ValueType,int16_t>;
      case DataType::UInt16BE:
        return __store_BE<ValueType,uint16_t>;
      case DataType::Int32LE:
        return __store_LE<ValueType,int32_t>;
      case DataType::UInt32LE:
        return __store_LE<ValueType,uint32_t>;
      case DataType::Int32BE:
        return __store_BE<ValueType,int32_t>;
      case DataType::UInt32BE:
        return __store_BE<ValueType,uint32_t>;
      case DataType::Int64LE:
        return __store_LE<ValueType,int64_t>;
      case DataType::UInt64LE:
        return __store_LE<ValueType,uint64_t>;
      case DataType::Int64BE:
        return __store_BE<ValueType,int64_t>;
      case DataType::UInt64BE:
        return __store_BE<ValueType,uint64_t>;
      case DataType::Float16LE:
        return __store_LE<ValueType,half_float::half>;
      case DataType::Float16BE:
        return __store_BE<ValueType,half_float::half>;
      case DataType::Float32LE:
        return __store_LE<ValueType,float>;
      case DataType::Float32BE:
        return __store_BE<ValueType,float>;
      case DataType::Float64LE:
        return __store_LE<ValueType,double>;
      case DataType::Float64BE:
        return __store_BE<ValueType,double>;
      case DataType::CFloat32LE:
        return __store_LE<ValueType,cfloat>;
      case DataType::CFloat32BE:
        return __store_BE<ValueType,cfloat>;
      case DataType::CFloat64LE:
        return __store_LE<ValueType,cdouble>;
      case DataType::CFloat64BE:
        return __store_BE<ValueType,cdouble>;
      default:
        throw Exception ("Unknown data type in request for store function");
    }
  }




  template <typename ValueType>
    typename std::enable_if<is_data_type<ValueType>::value, void>::type __set_fetch_store_scale_functions (
        std::function<ValueType(const void*,size_t,default_type,default_type)>& fetch_func,
        std::function<void(ValueType,void*,size_t,default_type,default_type)>& store_func,
        const DataType datatype) {

      switch (datatype()) {
        case DataType::Bit:
          fetch_func = __fetch_scale<ValueType,bool>;
          store_func = __scale_store<ValueType,bool>;
          return;
        case DataType::Int8:
          fetch_func = __fetch_scale<ValueType,int8_t>;
          store_func = __scale_store<ValueType,int8_t>;
          return;
        case DataType::UInt8:
          fetch_func = __fetch_scale<ValueType,uint8_t>;
          store_func = __scale_store<ValueType,uint8_t>;
          return;
        case DataType::Int16LE:
          fetch_func = __fetch_scale_LE<ValueType,int16_t>;
          store_func = __scale_store_LE<ValueType,int16_t>;
          return;
        case DataType::UInt16LE:
          fetch_func = __fetch_scale_LE<ValueType,uint16_t>;
          store_func = __scale_store_LE<ValueType,uint16_t>;
          return;
        case DataType::Int16BE:
          fetch_func = __fetch_scale_BE<ValueType,int16_t>;
          store_func = __scale_store_BE<ValueType,int16_t>;
          return;
        case DataType::UInt16BE:
          fetch_func = __fetch_scale_BE<ValueType,uint16_t>;
          store_func = __scale_store_BE<ValueType,uint16_t>;
          return;
        case DataType::Int32LE:
          fetch_func = __fetch_scale_LE<ValueType,int32_t>;
          store_func = __scale_store_LE<ValueType,int32_t>;
          return;
        case DataType::UInt32LE:
          fetch_func = __fetch_scale_LE<ValueType,uint32_t>;
          store_func = __scale_store_LE<ValueType,uint32_t>;
          return;
        case DataType::Int32BE:
          fetch_func = __fetch_scale_BE<ValueType,int32_t>;
          store_func = __scale_store_BE<ValueType,int32_t>;
          return;
        case DataType::UInt32BE:
          fetch_func = __fetch_scale_BE<ValueType,uint32_t>;
          store_func = __scale_store_BE<ValueType,uint32_t>;
          return;
        case DataType::Int64LE:
          fetch_func = __fetch_scale_LE<ValueType,int64_t>;
          store_func = __scale_store_LE<ValueType,int64_t>;
          return;
        case DataType::UInt64LE:
          fetch_func = __fetch_scale_LE<ValueType,uint64_t>;
          store_func = __scale_store_LE<ValueType,uint64_t>;
          return;
        case DataType::Int64BE:
          fetch_func = __fetch_scale_BE<ValueType,int64_t>;
          store_func = __scale_store_BE<ValueType,int64_t>;
          return;
        case DataType::UInt64BE:
          fetch_func = __fetch_scale_BE<ValueType,uint64_t>;
          store_func = __scale_store_BE<ValueType,uint64_t>;
          return;
        case DataType::Float16LE:
          fetch_func = __fetch_scale_LE<ValueType,half_float::half>;
          store_func = __scale_store_LE<ValueType,half_float::half>;
          return;
        case DataType::Float16BE:
          fetch_func = __fetch_scale_BE<ValueType,half_float::half>;
          store_func = __scale_store_BE<ValueType,half_float::half>;
          return;
        case DataType::Float32LE:
          fetch_func = __fetch_scale_LE<ValueType,float>;
          store_func = __scale_store_LE<ValueType,float>;
          return;
        case DataType::Float32BE:
          fetch_func = __fetch_scale_BE<ValueType,float>;
          store_func = __scale_store_BE<ValueType,float>;
          return;
        case DataType::Float64LE:
          fetch_func = __fetch_scale_LE<ValueType,double>;
          store_func = __scale_store_LE<ValueType,double>;
          return;
        case DataType::Float64BE:
          fetch_func = __fetch_scale_BE<ValueType,double>;
          store_func = __scale_store_BE<ValueType,double>;
          return;
        case DataType::CFloat32LE:
          fetch_func = __fetch_scale_LE<ValueType,cfloat>;
          store_func = __scale_store_LE<ValueType,cfloat>;
          return;
        case DataType::CFloat32BE:
          fetch_func = __fetch_scale_BE<ValueType,cfloat>;
          store_func = __scale_store_BE<ValueType,cfloat>;
          return;
        case DataType::CFloat64LE:
          fetch_func = __fetch_scale_LE<ValueType,cdouble>;
          store_func = __scale_store_LE<ValueType,cdouble>;
          return;
        case DataType::CFloat64BE:
          fetch_func = __fetch_scale_BE<ValueType,cdouble>;
          store_func = __scale_store_BE<ValueType,cdouble>;
          return;
        default:
          throw Exception ("invalid data type in image header");
      }
    }

  // explicit instantiation of fetch/store methods for all types:
#define __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(ValueType) \
  template std::function<ValueType(const void*,size_t)> __set_fetch_function<ValueType> (const DataType datatype); \
  template std::function<void(ValueType,void*,size_t)> __set_store_function<ValueType> (const DataType datatype); \
  template void __set_fetch_store_scale_functions<ValueType> ( \
      std::function<ValueType(const void*,size_t,default_type,default_type)>& fetch_func, \
      std::function<void(ValueType,void*,size_t,default_type,default_type)>& store_func, \
      const DataType datatype)

  __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(bool);
  __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(uint8_t);
  __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(int8_t);
  __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(uint16_t);
  __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(int16_t);
  __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(uint32_t);
  __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(int32_t);
  __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(uint64_t);
  __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(int64_t);
  __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(half_float::half);
  __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(float);
  __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(double);
  __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(cfloat);
  __DEFINE_FETCH_STORE_FUNCTIONS_FOR_TYPE(cdouble);


}


