/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 05/08/09.

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

#include <limits>

#include "app.h"
#include "get_set.h"
#include "image/voxel.h"
#include "image/misc.h"

#define MAX_FILES_PER_IMAGE 256U

namespace MR {
  namespace Image {

    Voxel::SharedInfo::SharedInfo (Header& parent) : H (parent)
    {
      if (H.files.empty()) throw Exception ("no files specified in header for image \"" + H.name() + "\"");
      assert (H.handler);
      H.handler->execute();
      H.axes.get_strides (start, stride);
    }







    namespace {
      template <typename T> float __get   (const void* data, size_t i) { return (MR::get<T> (data, i)); }
      template <typename T> float __getLE (const void* data, size_t i) { return (MR::getLE<T> (data, i)); }
      template <typename T> float __getBE (const void* data, size_t i) { return (MR::getBE<T> (data, i)); }

      template <typename T> void __put   (float val, void* data, size_t i) { return (MR::put<T> (val, data, i)); }
      template <typename T> void __putLE (float val, void* data, size_t i) { return (MR::putLE<T> (val, data, i)); }
      template <typename T> void __putBE (float val, void* data, size_t i) { return (MR::putBE<T> (val, data, i)); }
    }



    void Voxel::SharedInfo::init ()
    {
      // TODO: proper complex data support
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



