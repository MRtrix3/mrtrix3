/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __raw_h__
#define __raw_h__

/** \defgroup Binary Binary access functions
 * \brief functions to provide easy access to binary data. */

#include <atomic>

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

namespace MR
{

  /** \addtogroup Binary
   * @{ */

  namespace ByteOrder
  {

    template <typename ValueType>
      inline typename std::enable_if<std::is_fundamental<ValueType>::value && sizeof(ValueType) == 1, ValueType>::type swap (ValueType v) {
        return v; 
      }

    template <typename ValueType>
      inline typename std::enable_if<std::is_fundamental<ValueType>::value && sizeof(ValueType) == 2, ValueType>::type swap (ValueType v) {
        union {
          ValueType v;
          uint8_t i[2];
        } val = { v };
        std::swap (val.i[0], val.i[1]);
        return val.v;
      }

    template <typename ValueType>
      inline typename std::enable_if<std::is_fundamental<ValueType>::value && sizeof(ValueType) == 4, ValueType>::type swap (ValueType v) {
        union {
          ValueType v;
          uint8_t i[4];
        } val = { v };
        std::swap (val.i[0], val.i[3]);
        std::swap (val.i[1], val.i[2]);
        return val.v;
      }

    template <typename ValueType>
      inline typename std::enable_if<std::is_fundamental<ValueType>::value && sizeof(ValueType) == 8, ValueType>::type swap (ValueType v) {
        union {
          ValueType v;
          uint8_t i[8];
        } val = { v };
        std::swap (val.i[0], val.i[7]);
        std::swap (val.i[1], val.i[6]);
        std::swap (val.i[2], val.i[5]);
        std::swap (val.i[3], val.i[4]);
        return val.v;
      }

    template <typename ValueType>
      inline typename std::enable_if<is_complex<ValueType>::value, ValueType>::type swap (ValueType v) { return { swap (v.real()), swap (v.imag()) }; } 

    template <typename ValueType>
      inline ValueType LE (ValueType v) { return TO_LE (v); }

    template <typename ValueType>
      inline ValueType BE (ValueType v) { return TO_BE (v); }

    template <typename ValueType> 
      inline ValueType swap (const ValueType value, bool is_big_endian) { return is_big_endian ? BE (value) : LE (value); }

  }

  namespace Raw {

    namespace {
      template <typename ValueType> ValueType* as (void* p) { return reinterpret_cast<ValueType*> (p); }
      template <typename ValueType> const ValueType* as (const void* p) { return reinterpret_cast<const ValueType*> (p); }
    }


    // GET from pointer:
    template <typename ValueType> 
      inline ValueType fetch_LE (const void* address) { return ByteOrder::LE (*as<ValueType> (address)); }

    template <typename ValueType> 
      inline ValueType fetch_BE (const void* address) { return ByteOrder::BE (*as<ValueType> (address)); }

    template <typename ValueType> 
      inline ValueType fetch_ (const void* address, bool is_big_endian = MRTRIX_IS_BIG_ENDIAN) { return ByteOrder::swap (*as<ValueType>(address), is_big_endian); }

    template <typename ValueType> 
      inline ValueType fetch__native (const void* address) { return *as<ValueType>(address); }

    // PUT at pointer:
    template <typename ValueType> 
      inline void store_LE (const ValueType value, void* address) { *as<ValueType>(address) = ByteOrder::LE (value); }

    template <typename ValueType> 
      inline void store_BE (const ValueType value, void* address) { *as<ValueType>(address) = ByteOrder::BE (value); }

    template <typename ValueType> 
      inline void store (const ValueType value, void* address, bool is_big_endian = MRTRIX_IS_BIG_ENDIAN) { *as<ValueType>(address) = ByteOrder::swap (value, is_big_endian); }

    template <typename ValueType> 
      inline void store_native (const ValueType value, void* address) { *as<ValueType>(address) = value; }



    //! fetch \a value in little-endian format from offset \a i from \a data 
    template <typename ValueType> 
      inline ValueType fetch_LE (const void* data, size_t i) { return ByteOrder::LE (as<ValueType>(data)[i]); }

    //! fetch \a value in big-endian format from offset \a i from \a data 
    template <typename ValueType> 
      inline ValueType fetch_BE (const void* data, size_t i) { return ByteOrder::BE (as<ValueType>(data)[i]); }

    //! fetch \a value in format \a is_big_endian from offset \a i from \a data 
    template <typename ValueType> 
      inline ValueType fetch (const void* data, size_t i, bool is_big_endian = MRTRIX_IS_BIG_ENDIAN) { return ByteOrder::swap (as<ValueType>(data)[i], is_big_endian); }

    //! fetch \a value in native format from offset \a i from \a data 
    template <typename ValueType> 
      inline ValueType fetch_native (const void* data, size_t i) { return as<ValueType>(data)[i]; }



    //! store \a value in little-endian format at offset \a i from \a data 
    template <typename ValueType> 
      inline void store_LE (const ValueType value, void* data, size_t i) { as<ValueType>(data)[i] = ByteOrder::LE (value); }

    //! store \a value in big-endian format at offset \a i from \a data 
    template <typename ValueType> 
      inline void store_BE (const ValueType value, void* data, size_t i) { as<ValueType>(data)[i] = ByteOrder::BE (value); }

    //! store \a value in format \a is_big_endian at offset \a i from \a data 
    template <typename ValueType> 
      inline void store (const ValueType value, void* data, size_t i, bool is_big_endian = MRTRIX_IS_BIG_ENDIAN) { as<ValueType>(data)[i] = ByteOrder::swap (value, is_big_endian); }

    //! store \a value in native format at offset \a i from \a data 
    template <typename ValueType> 
      inline void store_native (const ValueType value, void* data, size_t i) { as<ValueType>(data)[i] = value; }


    //! \cond skip


    template <> 
      inline bool fetch_native<bool> (const void* data, size_t i) { return ( as<uint8_t>(data)[i/8]) & (BITMASK >> i%8 ); }

    template <> 
      inline void store_native<bool> (const bool value, void* data, size_t i) {
        std::atomic<uint8_t>* at = reinterpret_cast<std::atomic<uint8_t>*> (as<uint8_t>(data) + (i/8));
        uint8_t prev = *at, new_value;
        do {
          if (value) new_value = prev | (BITMASK >> i%8);
          else new_value = prev & ~(BITMASK >> i%8);
        } while (!at->compare_exchange_weak (prev, new_value));
      }


    template <> 
      inline bool fetch<bool> (const void* data, size_t i, bool) { return fetch_native<bool> (data, i); }

    template <> 
      inline void store<bool> (const bool value, void* data, size_t i, bool) { store_native<bool> (value, data, i); }

    //! \endcond
  }

  /** @} */

}


#endif
