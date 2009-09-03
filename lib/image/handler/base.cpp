/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 03/09/09.

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

#include "image/handler/base.h"
#include "image/header.h"

namespace MR {
  namespace Image {
    namespace Handler {

      void Base::prepare ()
      {
        assert (addresses.empty());

        execute(); 
        H.axes.get_strides (start_, stride_);

        switch (H.datatype()()) {
          case DataType::Bit:        get_func = &__get<bool>;       put_func = &__put<bool>;       return;
          case DataType::Int8:       get_func = &__get<int8_t>;     put_func = &__put<int8_t>;     return;
          case DataType::UInt8:      get_func = &__get<uint8_t>;    put_func = &__put<uint8_t>;    return;
          case DataType::Int16LE:    get_func = &__getLE<int16_t>;  put_func = &__putLE<int16_t>;  return;
          case DataType::UInt16LE:   get_func = &__getLE<uint16_t>; put_func = &__putLE<uint16_t>; return;
          case DataType::Int16BE:    get_func = &__getBE<int16_t>;  put_func = &__putBE<int16_t>;  return;
          case DataType::UInt16BE:   get_func = &__getBE<uint16_t>; put_func = &__putBE<uint16_t>; return;
          case DataType::Int32LE:    get_func = &__getLE<int32_t>;  put_func = &__putLE<int32_t>;  return;
          case DataType::UInt32LE:   get_func = &__getLE<uint32_t>; put_func = &__putLE<uint32_t>; return;
          case DataType::Int32BE:    get_func = &__getBE<int32_t>;  put_func = &__putBE<int32_t>;  return;
          case DataType::UInt32BE:   get_func = &__getBE<uint32_t>; put_func = &__putBE<uint32_t>; return;
          case DataType::Float32LE:  get_func = &__getLE<float>;    put_func = &__putLE<float>;    return;
          case DataType::Float32BE:  get_func = &__getBE<float>;    put_func = &__putBE<float>;    return;
          case DataType::Float64LE:  get_func = &__getLE<double>;   put_func = &__putLE<double>;   return;
          case DataType::Float64BE:  get_func = &__getBE<double>;   put_func = &__putBE<double>;   return;
          default: throw Exception ("invalid data type in image header");
        }
      }

    }
  }
}

