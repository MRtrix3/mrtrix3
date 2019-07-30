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


#include "app.h"
#include "datatype.h"

namespace MR
{

  constexpr uint8_t DataType::Attributes;
  constexpr uint8_t DataType::Type;

  constexpr uint8_t DataType::Complex;
  constexpr uint8_t DataType::Signed;
  constexpr uint8_t DataType::LittleEndian;
  constexpr uint8_t DataType::BigEndian;
  constexpr uint8_t DataType::Undefined;

  constexpr uint8_t DataType::Bit;
  constexpr uint8_t DataType::UInt8;
  constexpr uint8_t DataType::UInt16;
  constexpr uint8_t DataType::UInt32;
  constexpr uint8_t DataType::Float32;
  constexpr uint8_t DataType::Float64;
  constexpr uint8_t DataType::Int8;
  constexpr uint8_t DataType::Int16;
  constexpr uint8_t DataType::Int16LE;
  constexpr uint8_t DataType::UInt16LE;
  constexpr uint8_t DataType::Int16BE;
  constexpr uint8_t DataType::UInt16BE;
  constexpr uint8_t DataType::Int32;
  constexpr uint8_t DataType::Int32LE;
  constexpr uint8_t DataType::UInt32LE;
  constexpr uint8_t DataType::Int32BE;
  constexpr uint8_t DataType::UInt32BE;
  constexpr uint8_t DataType::Int64;
  constexpr uint8_t DataType::Int64LE;
  constexpr uint8_t DataType::UInt64LE;
  constexpr uint8_t DataType::Int64BE;
  constexpr uint8_t DataType::UInt64BE;
  constexpr uint8_t DataType::Float32LE;
  constexpr uint8_t DataType::Float32BE;
  constexpr uint8_t DataType::Float64LE;
  constexpr uint8_t DataType::Float64BE;
  constexpr uint8_t DataType::CFloat32;
  constexpr uint8_t DataType::CFloat32LE;
  constexpr uint8_t DataType::CFloat32BE;
  constexpr uint8_t DataType::CFloat64;
  constexpr uint8_t DataType::CFloat64LE;
  constexpr uint8_t DataType::CFloat64BE;
  constexpr uint8_t DataType::Native;

  const char* DataType::identifiers[] = {
    "float32", "float32le", "float32be", "float64", "float64le", "float64be",
    "int64", "uint64", "int64le", "uint64le", "int64be", "uint64be",
    "int32", "uint32", "int32le", "uint32le", "int32be", "uint32be",
    "int16", "uint16", "int16le", "uint16le", "int16be", "uint16be",
    "cfloat32", "cfloat32le", "cfloat32be", "cfloat64", "cfloat64le", "cfloat64be",
    "int8", "uint8", "bit", NULL
  };


  DataType DataType::parse (const std::string& spec)
  {
    std::string str (lowercase (spec));

    if (str == "float32") 
      return Float32;
    if (str == "float32le") 
      return Float32LE;
    if (str == "float32be") 
      return Float32BE;

    if (str == "float64") 
      return Float64;
    if (str == "float64le")
      return Float64LE;
    if (str == "float64be") 
      return Float64BE;

    if (str == "int64")
      return Int64;
    if (str == "uint64")
      return UInt64;
    if (str == "int64le")
      return Int64LE;
    if (str == "uint64le")
      return UInt64LE;
    if (str == "int64be")
      return Int64BE;
    if (str == "uint64be")
      return UInt64BE;

    if (str == "int32") 
      return Int32;
    if (str == "uint32") 
      return UInt32;
    if (str == "int32le") 
      return Int32LE;
    if (str == "uint32le")
      return UInt32LE;
    if (str == "int32be") 
      return Int32BE;
    if (str == "uint32be") 
      return UInt32BE;

    if (str == "int16") 
      return Int16;
    if (str == "uint16") 
      return UInt16;
    if (str == "int16le") 
      return Int16LE;
    if (str == "uint16le")
      return UInt16LE;
    if (str == "int16be")
      return Int16BE;
    if (str == "uint16be")
      return UInt16BE;

    if (str == "cfloat32") 
      return CFloat32;
    if (str == "cfloat32le")
      return CFloat32LE;
    if (str == "cfloat32be")
      return CFloat32BE;

    if (str == "cfloat64") 
      return CFloat64;
    if (str == "cfloat64le")
      return CFloat64LE;
    if (str == "cfloat64be")
      return CFloat64BE;

    if (str == "int8")
      return Int8;
    if (str == "uint8")
      return UInt8;

    if (str == "bit")
      return Bit;

    throw Exception ("invalid data type \"" + spec + "\"");
  }




  size_t DataType::bits () const
  {
    switch (dt & Type) {
      case Bit:
        return 1;
      case UInt8:
        return 8;
      case UInt16:
        return 16;
      case UInt32:
        return 32;
      case UInt64:
        return 64;
      case Float32:
        return is_complex() ? 64 : 32;
      case Float64:
        return is_complex() ? 128 : 64;
      default:
        throw Exception ("invalid datatype specifier");
    }
    return 0;
  }






  const char* DataType::description() const
  {
    switch (dt) {
      case Bit:
        return "bitwise";

      case Int8:
        return "signed 8 bit integer";
      case UInt8:
        return "unsigned 8 bit integer";

      case Int16LE:
        return "signed 16 bit integer (little endian)";
      case UInt16LE:
        return "unsigned 16 bit integer (little endian)";
      case Int16BE:
        return "signed 16 bit integer (big endian)";
      case UInt16BE:
        return "unsigned 16 bit integer (big endian)";

      case Int32LE:
        return "signed 32 bit integer (little endian)";
      case UInt32LE:
        return "unsigned 32 bit integer (little endian)";
      case Int32BE:
        return "signed 32 bit integer (big endian)";
      case UInt32BE:
        return "unsigned 32 bit integer (big endian)";

      case Int64LE:
        return "signed 64 bit integer (little endian)";
      case UInt64LE:
        return "unsigned 64 bit integer (little endian)";
      case Int64BE:
        return "signed 64 bit integer (big endian)";
      case UInt64BE:
        return "unsigned 64 bit integer (big endian)";

      case Float32LE:
        return "32 bit float (little endian)";
      case Float32BE:
        return "32 bit float (big endian)";

      case Float64LE:
        return "64 bit float (little endian)";
      case Float64BE:
        return "64 bit float (big endian)";

      case CFloat32LE:
        return "Complex 32 bit float (little endian)";
      case CFloat32BE:
        return "Complex 32 bit float (big endian)";

      case CFloat64LE:
        return "Complex 64 bit float (little endian)";
      case CFloat64BE:
        return "Complex 64 bit float (big endian)";

      case Undefined:
        return "undefined";

      default:
        return "invalid data type";
    }

    return nullptr;
  }







  const char* DataType::specifier() const
  {
    switch (dt) {
      case Bit:
        return "Bit";

      case Int8:
        return "Int8";
      case UInt8:
        return "UInt8";

      case Int16LE:
        return "Int16LE";
      case UInt16LE:
        return "UInt16LE";
      case Int16BE:
        return "Int16BE";
      case UInt16BE:
        return "UInt16BE";

      case Int32LE:
        return ("Int32LE");
      case UInt32LE:
        return ("UInt32LE");
      case Int32BE:
        return "Int32BE";
      case UInt32BE:
        return "UInt32BE";

      case Int64LE:
        return ("Int64LE");
      case UInt64LE:
        return ("UInt64LE");
      case Int64BE:
        return "Int64BE";
      case UInt64BE:
        return "UInt64BE";

      case Float32LE:
        return "Float32LE";
      case Float32BE:
        return "Float32BE";

      case Float64LE:
        return "Float64LE";
      case Float64BE:
        return "Float64BE";

      case CFloat32LE:
        return "CFloat32LE";
      case CFloat32BE:
        return "CFloat32BE";

      case CFloat64LE:
        return "CFloat64LE";
      case CFloat64BE:
        return "CFloat64BE";

      case Int16:
        return "Int16";
      case UInt16:
        return "UInt16";
      case Int32:
        return "Int32";
      case UInt32:
        return "UInt32";
      case Int64:
        return "Int64";
      case UInt64:
        return "UInt64";
      case Float32:
        return "Float32";
      case Float64:
        return "Float64";
      case CFloat32:
        return "CFloat32";
      case CFloat64:
        return "CFloat64";

      case Undefined:
        return "Undefined";

      default:
        return "invalid";
    }

    return NULL;
  }


  DataType DataType::from_command_line (DataType default_datatype)
  {
    auto opt = App::get_options ("datatype");
    if (opt.size()) 
      default_datatype = parse (opt[0][0]);
    return default_datatype;
  }


  App::OptionGroup DataType::options ()
  {
    using namespace App;
    return OptionGroup ("Data type options") 
      + Option ("datatype", "specify output image data type. "
          "Valid choices are: " + join (identifiers, ", ") + ".")
      + Argument ("spec").type_choice (identifiers);
  }

}

