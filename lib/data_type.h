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

*/

#ifndef __data_type_h__
#define __data_type_h__

#include "mrtrix.h"

namespace MR {

  class DataType {
    public:
      DataType () : dt (DataType::Native) { }
      DataType (uint8_t type) : dt (type) { }
      DataType (const DataType& DT) : dt (DT.dt) { }
      uint8_t&       operator() () { return (dt); }
      const uint8_t& operator() () const { return (dt); }
      bool operator== (uint8_t type) const { return (dt == type); }
      bool operator!= (uint8_t type) const { return (dt != type); }
      bool operator== (const DataType DT) const { return (dt == DT.dt); }
      bool operator!= (const DataType DT) const { return (dt != DT.dt); }
      const DataType& operator= (const DataType DT) { dt = DT.dt; return (*this); }

      bool is (uint8_t type) const { return (dt == type); }
      bool is_complex () const { return (dt & Complex); }
      bool is_signed () const { return (dt & Signed); }
      bool is_little_endian () const { return (dt & LittleEndian); }
      bool is_big_endian () const { return (dt & BigEndian); }
      void set_byte_order_native () {
        if ( dt != Bit && dt != Int8 && dt != UInt8 ) {
          if ( !is_little_endian() && !is_big_endian() ) {
#ifdef BYTE_ORDER_BIG_ENDIAN
            dt |= BigEndian;
#else
            dt |= LittleEndian;
#endif
          }
        }
      }

      void         parse (const std::string& spec);
      size_t       bits () const;
      size_t       bytes () const { return ((bits()+7)/8); }
      const char*  description () const;
      const char*  specifier () const;

      void set_flag (uint8_t flag) { dt |= flag; }
      void unset_flag (uint8_t flag) { dt &= ~flag; }



      static const uint8_t     Attributes    = 0xF0U;
      static const uint8_t     Type          = 0x0FU;

      static const uint8_t     Complex       = 0x10U;
      static const uint8_t     Signed        = 0x20U;
      static const uint8_t     LittleEndian  = 0x40U;
      static const uint8_t     BigEndian     = 0x80U;

      static const uint8_t     Undefined     = 0x00U;
      static const uint8_t     Bit           = 0x01U;
      static const uint8_t     UInt8         = 0x02U;
      static const uint8_t     UInt16        = 0x03U;
      static const uint8_t     UInt32        = 0x04U;
      static const uint8_t     Float32       = 0x05U;
      static const uint8_t     Float64       = 0x06U;

      static const uint8_t     Int8          = UInt8  | Signed;
      static const uint8_t     Int16         = UInt16 | Signed;
      static const uint8_t     Int16LE       = UInt16 | Signed | LittleEndian;
      static const uint8_t     UInt16LE      = UInt16 | LittleEndian;
      static const uint8_t     Int16BE       = UInt16 | Signed | BigEndian;
      static const uint8_t     UInt16BE      = UInt16 | BigEndian;
      static const uint8_t     Int32         = UInt32 | Signed;
      static const uint8_t     Int32LE       = UInt32 | Signed | LittleEndian;
      static const uint8_t     UInt32LE      = UInt32 | LittleEndian;
      static const uint8_t     Int32BE       = UInt32 | Signed | BigEndian;
      static const uint8_t     UInt32BE      = UInt32 | BigEndian;
      static const uint8_t     Float32LE     = Float32 | LittleEndian;
      static const uint8_t     Float32BE     = Float32 | BigEndian;
      static const uint8_t     Float64LE     = Float64 | LittleEndian;
      static const uint8_t     Float64BE     = Float64 | BigEndian;
      static const uint8_t     CFloat32      = Complex | Float32;
      static const uint8_t     CFloat32LE    = Complex | Float32 | LittleEndian;
      static const uint8_t     CFloat32BE    = Complex | Float32 | BigEndian;
      static const uint8_t     CFloat64      = Complex | Float64;
      static const uint8_t     CFloat64LE    = Complex | Float64 | LittleEndian;
      static const uint8_t     CFloat64BE    = Complex | Float64 | BigEndian;

      static const uint8_t     Native        = Float32 | 
#ifdef BYTE_ORDER_BIG_ENDIAN
        BigEndian;
#else
        LittleEndian;
#endif

        static const char* identifiers[];

    protected:
      uint8_t dt;

  };

}

#endif

