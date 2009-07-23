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
    protected:
      uint8_t dt;

    public:
      DataType ();
      DataType (uint8_t type);
      DataType (const DataType& DT);

      uint8_t&                operator() ();
      const uint8_t&          operator() () const;
      bool                   operator== (uint8_t type) const;
      bool                   operator!= (uint8_t type) const;
      bool                   operator== (const DataType DT) const;
      bool                   operator!= (const DataType DT) const;
      const DataType&        operator= (const DataType DT);

      bool                   is (uint8_t type) const;
      bool                   is_complex () const;
      bool                   is_signed () const;
      bool                   is_little_endian () const;
      bool                   is_big_endian () const;
      void                   set_byte_order_native ();

      void                   parse (const std::string& spec);
      uint                  bits () const;
      uint                  bytes () const;
      const char*           description () const;
      const char*           specifier () const;

      void                   set_flag (uint8_t flag);
      void                   unset_flag (uint8_t flag);



      static const uint8_t     ComplexNumber = 0x10U;
      static const uint8_t     Signed        = 0x20U;
      static const uint8_t     LittleEndian  = 0x40U;
      static const uint8_t     BigEndian     = 0x80U;
      static const uint8_t     Text          = 0xFFU;
      static const uint8_t     GroupStart    = 0xFEU;
      static const uint8_t     GroupEnd      = 0xFDU;

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
      static const uint8_t     CFloat32      = ComplexNumber | Float32;
      static const uint8_t     CFloat32LE    = ComplexNumber | Float32 | LittleEndian;
      static const uint8_t     CFloat32BE    = ComplexNumber | Float32 | BigEndian;
      static const uint8_t     CFloat64      = ComplexNumber | Float64;
      static const uint8_t     CFloat64LE    = ComplexNumber | Float64 | LittleEndian;
      static const uint8_t     CFloat64BE    = ComplexNumber | Float64 | BigEndian;

      static const uint8_t     Native        = Float32 | 
#ifdef BYTE_ORDER_BIG_ENDIAN
        BigEndian;
#else
        LittleEndian;
#endif
  };








  inline DataType::DataType () :
#ifdef BYTE_ORDER_BIG_ENDIAN
      dt (DataType::Float32BE)
#else
      dt (DataType::Float32LE)
#endif
  { }

  inline DataType::DataType (uint8_t type) : dt (type)               { }
  inline DataType::DataType (const DataType& DT) : dt (DT.dt)       { }
  inline uint8_t& DataType::operator() ()                            { return (dt); }
  inline const uint8_t& DataType::operator() () const                { return (dt); }
  inline bool DataType::operator== (uint8_t type) const              { return (dt == type); }
  inline bool DataType::operator!= (uint8_t type) const              { return (dt != type); }
  inline bool DataType::operator== (const DataType DT) const        { return (dt == DT.dt); }
  inline bool DataType::operator!= (const DataType DT) const        { return (dt != DT.dt); }
  inline const DataType& DataType::operator= (const DataType DT)    { dt = DT.dt; return (*this); }
  inline uint DataType::bytes () const                             { return ((bits()+7)/8); }
  inline bool DataType::is (uint8_t type) const                      { return (dt == type); }
  inline bool DataType::is_complex () const                         { return (dt & ComplexNumber); }
  inline bool DataType::is_signed () const                          { return (dt & Signed); }
  inline bool DataType::is_little_endian () const                   { return (dt & LittleEndian); }
  inline bool DataType::is_big_endian() const                       { return (dt & BigEndian); }
  inline void DataType::set_flag (uint8_t flag)                      { dt |= flag; }
  inline void DataType::unset_flag (uint8_t flag)                    { dt &= ~flag; }
  inline void DataType::set_byte_order_native ()
  {
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

}

#endif

