/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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


    31-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * replace get::T and put::T() methods with template get<T>() & put<T>() methods
    * add get/put template specialisations for bool, int8 and uint8
    * remove obsolete ArrayXX classes
    * move MR::ByteOrder namespace & methods from lib/mrtrix.h to here

*/

#ifndef __get_set_h__
#define __get_set_h__

/** \defgroup Binary Binary access functions
 * \brief functions to provide easy access to binary data. */

#include "mrtrix.h"

#define BITMASK 0x01U << 7

#ifdef BYTE_ORDER_IS_BIG_ENDIAN
#define MRTRIX_IS_BIG_ENDIAN true
#define TO_LE(v) swap(v)
#define TO_BE(v) v
#else 
#define MRTRIX_IS_BIG_ENDIAN false
#define TO_LE(v) v
#define TO_BE(v) swap(v)
#endif 

namespace MR {

  /** \addtogroup Binary 
   * @{ */

  namespace ByteOrder {
    namespace {
      template <typename T> inline void swap (T& a, T& b) throw () { T c (a); a = b; b = c; }
    }

    inline int16_t  swap (int16_t v)  { union { int16_t v; uint8_t i[2]; } val = { v }; swap (val.i[0], val.i[1]); return (val.v); }
    inline uint16_t swap (uint16_t v) { union { uint16_t v; uint8_t i[2]; } val = { v }; swap (val.i[0], val.i[1]); return (val.v); }
    inline int32_t  swap (int32_t v)  { union { int32_t v; uint8_t i[4]; } val = { v }; swap (val.i[0], val.i[3]); swap (val.i[1], val.i[2]); return (val.v); }
    inline uint32_t swap (uint32_t v) { union { uint32_t v; uint8_t i[4]; } val = { v }; swap (val.i[0], val.i[3]); swap (val.i[1], val.i[2]); return (val.v); }
    inline float32  swap (float32 v)  { union { float32 v; uint8_t i[4]; } val = { v }; swap (val.i[0], val.i[3]); swap (val.i[1], val.i[2]); return (val.v); }
    inline float64  swap (float64 v)  { union { float64 v; uint32_t i[2]; } val = { v }; uint32_t t = swap (val.i[0]); val.i[0] = swap (val.i[1]); val.i[1] = t; return (val.v); }

    inline int16_t LE (int16_t v)     { return (TO_LE (v)); }
    inline int16_t BE (int16_t v)     { return (TO_BE (v)); }
    inline uint16_t LE (uint16_t v)   { return (TO_LE (v)); }
    inline uint16_t BE (uint16_t v)   { return (TO_BE (v)); }
    inline int32_t LE (int32_t v)     { return (TO_LE (v)); }
    inline int32_t BE (int32_t v)     { return (TO_BE (v)); }
    inline uint32_t LE (uint32_t v)   { return (TO_LE (v)); }
    inline uint32_t BE (uint32_t v)   { return (TO_BE (v)); }
    inline float32 LE (float32 v)     { return (TO_LE (v)); }
    inline float32 BE (float32 v)     { return (TO_BE (v)); }
    inline float64 LE (float64 v)     { return (TO_LE (v)); }
    inline float64 BE (float64 v)     { return (TO_BE (v)); }

    template <typename T> inline T swap (const T value, bool is_big_endian) { return (is_big_endian ? BE (value) : LE (value)); }
  }


  template <typename T> inline T getLE (const void* address) { return (ByteOrder::LE (*((T*) address))); }
  template <typename T> inline T getBE (const void* address) { return (ByteOrder::BE (*((T*) address))); }
  template <typename T> inline T get (const void* address, bool is_big_endian = MRTRIX_IS_BIG_ENDIAN) 
  { return (ByteOrder::swap (*((T*) address), is_big_endian)); }

  template <typename T> inline void putLE (const T value, void* address) { *((T*) address) = ByteOrder::LE (value); }
  template <typename T> inline void putBE (const T value, void* address) { *((T*) address) = ByteOrder::BE (value); }
  template <typename T> inline void put (const T value, void* address, bool is_big_endian = MRTRIX_IS_BIG_ENDIAN) 
  { *((T*) address) = ByteOrder::swap (value, is_big_endian); }

  template <typename T> inline T getLE (const void* data, size_t i) { return (ByteOrder::LE (((T*) data)[i])); }
  template <typename T> inline T getBE (const void* data, size_t i) { return (ByteOrder::BE (((T*) data)[i])); }
  template <typename T> inline T get (const void* data, size_t i, bool is_big_endian = MRTRIX_IS_BIG_ENDIAN)
  { return (ByteOrder::swap (*((T*) data)[i], is_big_endian)); }

  template <typename T> inline void putLE (const T value, void* data, size_t i) { ((T*) data)[i] = ByteOrder::LE (value); }
  template <typename T> inline void putBE (const T value, void* data, size_t i) { ((T*) data)[i] = ByteOrder::BE (value); }
  template <typename T> inline void put (const T value, void* data, size_t i, bool is_big_endian = MRTRIX_IS_BIG_ENDIAN)
  { *((T*) data)[i] = ByteOrder::swap (value, is_big_endian); }


  //! \cond skip
  //
  template <> inline int8_t get<int8_t> (const void* address, bool is_big_endian) { return (*((int8_t*) address)); }
  template <> inline int8_t get<int8_t> (const void* data, size_t i, bool is_big_endian) { return (((int8_t *) data)[i]); }

  template <> inline void put<int8_t> (const int8_t value, void* address, bool is_big_endian) { *((int8_t*) address) = value; }
  template <> inline void put<int8_t> (const int8_t value, void* data, size_t i, bool is_big_endian) { ((int8_t *) data)[i] = value; }

  template <> inline uint8_t get<uint8_t> (const void* address, bool is_big_endian) { return (*((uint8_t*) address)); }
  template <> inline uint8_t get<uint8_t> (const void* data, size_t i, bool is_big_endian) { return (((uint8_t *) data)[i]); }

  template <> inline void put<uint8_t> (const uint8_t value, void* address, bool is_big_endian) { *((uint8_t*) address) = value; }
  template <> inline void put<uint8_t> (const uint8_t value, void* data, size_t i, bool is_big_endian) { ((uint8_t*) data)[i] = value; }


  template <> inline bool get<bool> (const void* data, size_t i, bool is_big_endian)
  { return ((((uint8_t*) data)[i/8]) & (BITMASK >> i%8)); } 

  template <> inline void put<bool> (const bool value, void* data, size_t i, bool is_big_endian)
  { 
    if (value) ((uint8_t*) data)[i/8] |= (BITMASK >> i%8); 
    else ((uint8_t*) data)[i/8] &= ~(BITMASK >> i%8); 
  }

  //! \endcond

  /** @} */

}


#endif
