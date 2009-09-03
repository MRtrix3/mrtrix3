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
#include "image/voxel.h"
#include "image/misc.h"

namespace MR {
  namespace Image {

    Voxel::Voxel (const Header& parent) : H (parent)
    {
      assert (H.handler);
      assert (H.ndim() < MAX_NDIM);

      H.handler->prepare();
      assert (H.handler->nsegments() < MAX_FILES_PER_IMAGE);

      num_dim = H.ndim();
      std::vector<ssize_t> stride_t;
      H.axes.get_strides (start, stride_t);

      for (size_t i = 0; i < ndim(); i++) {
        ax[i].x = 0;
        ax[i].stride = stride_t[i];
        ax[i].dim = H.dim(i);
        ax[i].vox = H.vox(i);
      }

      segsize = H.handler->voxels_per_segment();
      for (size_t i = 0; i < H.handler->nsegments(); i++)
        segment[i] = H.handler->segment(i);

      offset = start;

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



