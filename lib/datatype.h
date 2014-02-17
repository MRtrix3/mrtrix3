/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __data_type_h__
#define __data_type_h__

#include "args.h"
#include "mrtrix.h"

#ifdef Complex
# undef Complex
#endif

namespace MR
{

  class DataType
  {
    public:
      DataType () : dt (DataType::Native) { }
      DataType (uint8_t type) : dt (type) { }
      DataType (const DataType& DT) : dt (DT.dt) { }
      bool undefined () const {
        return dt == Undefined;
      }
      uint8_t& operator() () {
        return dt;
      }
      const uint8_t& operator() () const {
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
      const DataType& operator= (const DataType DT) {
        dt = DT.dt;
        return *this;
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
      static const uint8_t     UInt64        = 0x05U;
      static const uint8_t     Float32       = 0x06U;
      static const uint8_t     Float64       = 0x07U;


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
      static const uint8_t     Int64         = UInt64 | Signed;
      static const uint8_t     Int64LE       = UInt64 | Signed | LittleEndian;
      static const uint8_t     UInt64LE      = UInt64 | LittleEndian;
      static const uint8_t     Int64BE       = UInt64 | Signed | BigEndian;
      static const uint8_t     UInt64BE      = UInt64 | BigEndian;
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

      template <typename ValueType> static inline ValueType default_out_of_bounds_value () {
        return ValueType(0);
      }

    protected:
      uint8_t dt;

  };

  template <> inline float DataType::default_out_of_bounds_value ()
  {
    return NAN;
  }
  template <> inline double DataType::default_out_of_bounds_value ()
  {
    return NAN;
  }
  template <> inline cfloat DataType::default_out_of_bounds_value ()
  {
    return NAN;
  }
  template <> inline cdouble DataType::default_out_of_bounds_value ()
  {
    return NAN;
  }


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

