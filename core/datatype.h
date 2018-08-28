/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __data_type_h__
#define __data_type_h__

#include "cmdline_option.h"

#ifdef Complex
# undef Complex
#endif

namespace MR
{

  class DataType { NOMEMALIGN
    public:
      DataType () noexcept : dt (DataType::Native) { }
      DataType (uint8_t type) noexcept : dt (type) { }
      DataType (const DataType&) noexcept = default;
      DataType (DataType&&) noexcept = default;
      DataType& operator= (const DataType&) noexcept = default;
      DataType& operator= (DataType&&) noexcept = default;

      bool undefined () const {
        return dt == Undefined;
      }
      uint8_t operator() () const {
        return dt;
      }
      bool operator== (uint8_t type) const {
        return dt == type;
      }
      bool operator!= (uint8_t type) const {
        return dt != type;
      }
      bool operator== (const DataType DT) const {
        return dt == DT.dt;
      }
      bool operator!= (const DataType DT) const {
        return dt != DT.dt;
      }

      bool is (uint8_t type) const {
        return dt == type;
      }
      bool is_complex () const {
        return dt & Complex;
      }
      bool is_signed () const {
        return dt & Signed;
      }
      bool is_byte_order_native () {
        if (bits() <= 8)
          return true;
        if (!is_little_endian() && !is_big_endian())
          throw Exception ("byte order not set!");
#ifdef MRTRIX_BYTE_ORDER_BIG_ENDIAN
        return is_big_endian();
#else
        return is_little_endian();
#endif
      }
      bool is_little_endian () const {
        return dt & LittleEndian;
      }
      bool is_big_endian () const {
        return dt & BigEndian;
      }
      bool is_integer () const {
        const uint8_t type = dt & Type;
        return ((type == UInt8) || (type == UInt16) || (type == UInt32) || (type == UInt64));
      }
      bool is_floating_point () const {
        const uint8_t type = dt & Type;
        return ((type == Float32) || (type == Float64));
      }
      void set_byte_order_native () {
        if (dt != Bit && dt != Int8 && dt != UInt8) {
          if (!is_little_endian() && !is_big_endian()) {
#ifdef MRTRIX_BYTE_ORDER_BIG_ENDIAN
            dt |= BigEndian;
#else
            dt |= LittleEndian;
#endif
          }
        }
      }

      size_t bits () const;
      size_t bytes () const {
        return (bits() +7) /8;
      }
      const char*  description () const;
      const char*  specifier () const;

      void set_flag (uint8_t flag) {
        dt |= flag;
      }
      void unset_flag (uint8_t flag) {
        dt &= ~flag;
      }

      template <typename T>
        static DataType from () { return DataType::Undefined; }

      static DataType native (DataType dt) {
        dt.set_byte_order_native();
        return dt;
      }
      static DataType parse (const std::string& spec);
      static DataType from_command_line (DataType default_datatype = Undefined);


      static constexpr uint8_t     Attributes    = 0xF0U;
      static constexpr uint8_t     Type          = 0x0FU;

      static constexpr uint8_t     Complex       = 0x10U;
      static constexpr uint8_t     Signed        = 0x20U;
      static constexpr uint8_t     LittleEndian  = 0x40U;
      static constexpr uint8_t     BigEndian     = 0x80U;

      static constexpr uint8_t     Undefined     = 0x00U;
      static constexpr uint8_t     Bit           = 0x01U;
      static constexpr uint8_t     UInt8         = 0x02U;
      static constexpr uint8_t     UInt16        = 0x03U;
      static constexpr uint8_t     UInt32        = 0x04U;
      static constexpr uint8_t     UInt64        = 0x05U;
      static constexpr uint8_t     Float32       = 0x06U;
      static constexpr uint8_t     Float64       = 0x07U;


      static constexpr uint8_t     Int8          = UInt8  | Signed;
      static constexpr uint8_t     Int16         = UInt16 | Signed;
      static constexpr uint8_t     Int16LE       = UInt16 | Signed | LittleEndian;
      static constexpr uint8_t     UInt16LE      = UInt16 | LittleEndian;
      static constexpr uint8_t     Int16BE       = UInt16 | Signed | BigEndian;
      static constexpr uint8_t     UInt16BE      = UInt16 | BigEndian;
      static constexpr uint8_t     Int32         = UInt32 | Signed;
      static constexpr uint8_t     Int32LE       = UInt32 | Signed | LittleEndian;
      static constexpr uint8_t     UInt32LE      = UInt32 | LittleEndian;
      static constexpr uint8_t     Int32BE       = UInt32 | Signed | BigEndian;
      static constexpr uint8_t     UInt32BE      = UInt32 | BigEndian;
      static constexpr uint8_t     Int64         = UInt64 | Signed;
      static constexpr uint8_t     Int64LE       = UInt64 | Signed | LittleEndian;
      static constexpr uint8_t     UInt64LE      = UInt64 | LittleEndian;
      static constexpr uint8_t     Int64BE       = UInt64 | Signed | BigEndian;
      static constexpr uint8_t     UInt64BE      = UInt64 | BigEndian;
      static constexpr uint8_t     Float32LE     = Float32 | LittleEndian;
      static constexpr uint8_t     Float32BE     = Float32 | BigEndian;
      static constexpr uint8_t     Float64LE     = Float64 | LittleEndian;
      static constexpr uint8_t     Float64BE     = Float64 | BigEndian;
      static constexpr uint8_t     CFloat32      = Complex | Float32;
      static constexpr uint8_t     CFloat32LE    = Complex | Float32 | LittleEndian;
      static constexpr uint8_t     CFloat32BE    = Complex | Float32 | BigEndian;
      static constexpr uint8_t     CFloat64      = Complex | Float64;
      static constexpr uint8_t     CFloat64LE    = Complex | Float64 | LittleEndian;
      static constexpr uint8_t     CFloat64BE    = Complex | Float64 | BigEndian;

      static constexpr uint8_t     Native        = Float32 |
#ifdef MRTRIX_BYTE_ORDER_BIG_ENDIAN
          BigEndian;
#else
          LittleEndian;
#endif
      static App::OptionGroup options ();

      static const char* identifiers[];

      friend std::ostream& operator<< (std::ostream& stream, const DataType& dt) {
        stream << dt.specifier();
        return stream;
      }

    protected:
      uint8_t dt;

  };



  template <> inline DataType DataType::from<bool> ()
  {
    return DataType::Bit;
  }
  template <> inline DataType DataType::from<int8_t> ()
  {
    return DataType::Int8;
  }
  template <> inline DataType DataType::from<uint8_t> ()
  {
    return DataType::UInt8;
  }
  template <> inline DataType DataType::from<int16_t> ()
  {
    return DataType::native (DataType::Int16);
  }
  template <> inline DataType DataType::from<uint16_t> ()
  {
    return DataType::native (DataType::UInt16);
  }
  template <> inline DataType DataType::from<int32_t> ()
  {
    return DataType::native (DataType::Int32);
  }
  template <> inline DataType DataType::from<uint32_t> ()
  {
    return DataType::native (DataType::UInt32);
  }
  template <> inline DataType DataType::from<int64_t> ()
  {
    return DataType::native (DataType::Int64);
  }
  template <> inline DataType DataType::from<uint64_t> ()
  {
    return DataType::native (DataType::UInt64);
  }
  template <> inline DataType DataType::from<float> ()
  {
    return DataType::native (DataType::Float32);
  }
  template <> inline DataType DataType::from<double> ()
  {
    return DataType::native (DataType::Float64);
  }
  template <> inline DataType DataType::from<cfloat> ()
  {
    return DataType::native (DataType::CFloat32);
  }
  template <> inline DataType DataType::from<cdouble> ()
  {
    return DataType::native (DataType::CFloat64);
  }

}

#endif

