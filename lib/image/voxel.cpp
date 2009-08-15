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

    Voxel::SharedInfo::SharedInfo (Header& parent) : 
      H (parent),
      mem (NULL),
      segment (NULL),
      stride (NULL)
    {
      if (H.files.empty()) throw Exception ("no files specified in header for image \"" + H.name() + "\"");

      segsize = H.datatype().is_complex() ? 2 : 1;
      for (size_t i = 0; i < H.ndim(); i++) segsize *= H.dim(i); 
      segsize /= H.files.size();
      assert (segsize * H.files.size() == voxel_count (H));

      off64_t bps = (H.datatype().bits() * segsize + 7) / 8;
      if (H.files.size() * bps > std::numeric_limits<size_t>::max())
        throw Exception ("failed to allocate memory for image \"" + H.name() + "\"");

      debug ("mapping image \"" + H.name() + "\"...");

      if (H.files.size() > MAX_FILES_PER_IMAGE) {
        mem = new uint8_t [H.files.size() * bps];
        if (!mem) throw Exception ("failed to allocate memory for image \"" + H.name() + "\"");

        if (H.files_initialised) {
          for (size_t n = 0; n < H.files.size(); n++) {
            File::MMap file (H.files[n], false, bps);
            memcpy (mem + n*bps, file.address(), bps);
          }
        }
        else memset (mem, 0, H.files.size() * bps);

        if (H.datatype().bits() == 1 && H.files.size() > 1) {
          segment = new uint8_t* [H.files.size()];
          for (size_t n = 0; n < H.files.size(); n++) 
            segment[n] = mem + n*bps;
        }
      }
      else {
        files.resize (H.files.size());
        for (size_t n = 0; n < H.files.size(); n++) {
          files[n] = new File::MMap (H.files[n], H.readwrite, bps); 
        }
        if (files.size() > 1) {
          segment = new uint8_t* [H.files.size()];
          for (size_t n = 0; n < H.files.size(); n++) 
            segment[n] = files[n]->address();
        }
        else mem = files.front()->address();
      }

      stride = new ssize_t [H.ndim()];
      H.axes.get_strides (start, stride);

      assert (files.size() || mem);
      assert (mem || segment);
    }





    Voxel::SharedInfo::~SharedInfo ()
    {
      if (H.readwrite && !files.size()) {
        assert (mem);
        off64_t bps = (H.datatype().bits() * segsize + 7) / 8;

        for (size_t n = 0; n < H.files.size(); n++) {
          File::MMap file (H.files[n], true, bps);
          memcpy (segment ? segment[n] : mem + n*bps, file.address(), bps);
        }
      }

      if (mem) delete [] mem;
      if (segment) delete [] segment;
      if (stride) delete [] stride;
    }



    void Voxel::SharedInfo::init ()
    {
      // TODO: proper complex data support
      switch (H.datatype()()) {
        case DataType::Bit:        get_func = getBit;        put_func = putBit;        return;
        case DataType::Int8:       get_func = getInt8;       put_func = putInt8;       return;
        case DataType::UInt8:      get_func = getUInt8;      put_func = putUInt8;      return;
        case DataType::Int16LE:    get_func = getInt16LE;    put_func = putInt16LE;    return;
        case DataType::UInt16LE:   get_func = getUInt16LE;   put_func = putUInt16LE;   return;
        case DataType::Int16BE:    get_func = getInt16BE;    put_func = putInt16BE;    return;
        case DataType::UInt16BE:   get_func = getUInt16BE;   put_func = putUInt16BE;   return;
        case DataType::Int32LE:    get_func = getInt32LE;    put_func = putInt32LE;    return;
        case DataType::UInt32LE:   get_func = getUInt32LE;   put_func = putUInt32LE;   return;
        case DataType::Int32BE:    get_func = getInt32BE;    put_func = putInt32BE;    return;
        case DataType::UInt32BE:   get_func = getUInt32BE;   put_func = putUInt32BE;   return;
        case DataType::Float32LE:  get_func = getFloat32LE;  put_func = putFloat32LE;  return;
        case DataType::Float32BE:  get_func = getFloat32BE;  put_func = putFloat32BE;  return;
        case DataType::Float64LE:  get_func = getFloat64LE;  put_func = putFloat64LE;  return;
        case DataType::Float64BE:  get_func = getFloat64BE;  put_func = putFloat64BE;  return;
        default: throw Exception ("invalid data type in image header");
      }
    }






/*
    void Mapper::map (const Header& H)
    {
      assert (list.size() || mem);
      assert (segment == NULL);

      if (list.size() > DATAMAPPER_MAX_FILES || 
          ( optimised && ( list.size() > 1 || H.data_type != DataType::Native )) ) {

        if (H.data_type == DataType::Bit) optimised = true;

        info (std::string ("loading ") + ( optimised ? "and optimising " : "" ) + "image \"" + H.name + "\"..."); 

        bool read_only = list[0].fmap.is_read_only();

        size_t bpp = optimised ? sizeof (float32) : H.data_type.bytes();
        mem = new uint8_t [bpp*voxel_count(H.axes)];
        if (!mem) throw Exception ("failed to allocate memory for image data!");

        if (files_new) memset (mem, 0, bpp*voxel_count(H.axes));
        else {
          segsize = calc_segsize (H, list.size());

          for (size_t n = 0; n < list.size(); n++) {
            list[n].fmap.map (); 

            if (optimised) {
              float32* data = (float32*) mem + n*segsize;
              uint8_t*   fdata = list[n].start();
              for (size_t i = 0; i < segsize; i++) 
                data[i] = get_func (fdata, i); 
            } 
            else memcpy (mem + n*segsize*bpp, list[n].start(), segsize*bpp);

            list[n].fmap.unmap();
          }
        }

        if (temporary || read_only) list.clear();
      }

      if (mem) {
        segment = new uint8_t* [1];
        segment[0] = mem;
        segsize = optimised ? sizeof (float32) : H.data_type.bytes();
        segsize *= voxel_count(H.axes);
      }
      else {
        segment = new uint8_t* [list.size()];
        for (uint n = 0; n < list.size(); n++) {
          list[n].fmap.map();
          segment[n] = list[n].start();
        }
        segsize = calc_segsize (H, list.size());
      }


      debug ("data mapper for image \"" + H.name + "\" mapped with segment size = " + str (segsize) 
          + ( optimised ? " (optimised)" : "")); 
    }





    void Mapper::unmap (const Header& H)
    {
      if (mem && list.size()) {
        segsize = calc_segsize (H, list.size());
        if (!optimised) segsize *= H.data_type.bytes();

        info ("writing back data for image \"" + H.name + "\"...");
        for (uint n = 0; n < list.size(); n++) {
          bool err = false;
          try { list[n].fmap.map (); }
          catch (...) { err = true; error ("error writing data to file \"" + list[n].fmap.name() + "\""); }
          if (!err) {
            if (optimised) {
              const float32* data = (const float32*) mem + n*segsize;
              for (size_t i = 0; i < segsize; i++) 
                put_func (data[i], list[n].start(), i); 
            } 
            else memcpy (list[n].start(), mem + n*segsize, segsize);
            list[n].fmap.unmap();
          }
        }
      }

      delete [] mem;
      delete [] segment;
      mem = NULL;
      segment = NULL;
    }





*/










    float Voxel::SharedInfo::getBit       (const void* data, size_t i)  { return (MR::get<bool>       (data, i)); }
    float Voxel::SharedInfo::getInt8      (const void* data, size_t i)  { return (MR::get<int8_t>     (data, i)); }
    float Voxel::SharedInfo::getUInt8     (const void* data, size_t i)  { return (MR::get<uint8_t>    (data, i)); }
    float Voxel::SharedInfo::getInt16LE   (const void* data, size_t i)  { return (getLE<int16_t>  (data, i)); }
    float Voxel::SharedInfo::getUInt16LE  (const void* data, size_t i)  { return (getLE<uint16_t> (data, i)); }
    float Voxel::SharedInfo::getInt16BE   (const void* data, size_t i)  { return (getBE<int16_t>  (data, i)); }
    float Voxel::SharedInfo::getUInt16BE  (const void* data, size_t i)  { return (getBE<uint16_t> (data, i)); }
    float Voxel::SharedInfo::getInt32LE   (const void* data, size_t i)  { return (getLE<int32_t>  (data, i)); }
    float Voxel::SharedInfo::getUInt32LE  (const void* data, size_t i)  { return (getLE<uint32_t> (data, i)); }
    float Voxel::SharedInfo::getInt32BE   (const void* data, size_t i)  { return (getBE<int32_t>  (data, i)); }
    float Voxel::SharedInfo::getUInt32BE  (const void* data, size_t i)  { return (getBE<uint32_t> (data, i)); }
    float Voxel::SharedInfo::getFloat32LE (const void* data, size_t i)  { return (getLE<float> (data, i)); }
    float Voxel::SharedInfo::getFloat32BE (const void* data, size_t i)  { return (getBE<float> (data, i)); }
    float Voxel::SharedInfo::getFloat64LE (const void* data, size_t i)  { return (getLE<float64> (data, i)); }
    float Voxel::SharedInfo::getFloat64BE (const void* data, size_t i)  { return (getBE<float64> (data, i)); }

    void Voxel::SharedInfo::putBit       (float val, void* data, size_t i) { put<bool>      (bool(val), data, i); }
    void Voxel::SharedInfo::putInt8      (float val, void* data, size_t i) { put<int8_t>     (int8_t(val), data, i); }
    void Voxel::SharedInfo::putUInt8     (float val, void* data, size_t i) { put<uint8_t>    (uint8_t(val), data, i); }
    void Voxel::SharedInfo::putInt16LE   (float val, void* data, size_t i) { putLE<int16_t>  (int16_t(val), data, i); }
    void Voxel::SharedInfo::putUInt16LE  (float val, void* data, size_t i) { putLE<uint16_t> (uint16_t(val), data, i); }
    void Voxel::SharedInfo::putInt16BE   (float val, void* data, size_t i) { putBE<int16_t>  (int16_t(val), data, i); }
    void Voxel::SharedInfo::putUInt16BE  (float val, void* data, size_t i) { putBE<uint16_t> (uint16_t(val), data, i); }
    void Voxel::SharedInfo::putInt32LE   (float val, void* data, size_t i) { putLE<int32_t>  (int32_t(val), data, i); }
    void Voxel::SharedInfo::putUInt32LE  (float val, void* data, size_t i) { putLE<uint32_t> (uint32_t(val), data, i); }
    void Voxel::SharedInfo::putInt32BE   (float val, void* data, size_t i) { putBE<int32_t>  (int32_t(val), data, i); }
    void Voxel::SharedInfo::putUInt32BE  (float val, void* data, size_t i) { putBE<uint32_t> (uint32_t(val), data, i); }
    void Voxel::SharedInfo::putFloat32LE (float val, void* data, size_t i) { putLE<float> (float(val), data, i); }
    void Voxel::SharedInfo::putFloat32BE (float val, void* data, size_t i) { putBE<float> (float(val), data, i); }
    void Voxel::SharedInfo::putFloat64LE (float val, void* data, size_t i) { putLE<float64> (float64(val), data, i); }
    void Voxel::SharedInfo::putFloat64BE (float val, void* data, size_t i) { putBE<float64> (float64(val), data, i); }

  }
}



