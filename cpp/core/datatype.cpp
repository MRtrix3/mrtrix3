/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "datatype.h"
#include "app.h"
#include <vector>

namespace MR {

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
constexpr uint8_t DataType::Float16LE;
constexpr uint8_t DataType::Float16BE;
constexpr uint8_t DataType::Float32LE;
constexpr uint8_t DataType::Float32BE;
constexpr uint8_t DataType::Float64LE;
constexpr uint8_t DataType::Float64BE;
constexpr uint8_t DataType::CFloat16;
constexpr uint8_t DataType::CFloat16LE;
constexpr uint8_t DataType::CFloat16BE;
constexpr uint8_t DataType::CFloat32;
constexpr uint8_t DataType::CFloat32LE;
constexpr uint8_t DataType::CFloat32BE;
constexpr uint8_t DataType::CFloat64;
constexpr uint8_t DataType::CFloat64LE;
constexpr uint8_t DataType::CFloat64BE;
constexpr uint8_t DataType::Native;

const std::vector<std::string> DataType::identifiers = {
    "float16",    "float16le", "float16be",  "float32",    "float32le",  "float32be",  "float64",  "float64le",
    "float64be",  "int64",     "uint64",     "int64le",    "uint64le",   "int64be",    "uint64be", "int32",
    "uint32",     "int32le",   "uint32le",   "int32be",    "uint32be",   "int16",      "uint16",   "int16le",
    "uint16le",   "int16be",   "uint16be",   "cfloat16",   "cfloat16le", "cfloat16be", "cfloat32", "cfloat32le",
    "cfloat32be", "cfloat64",  "cfloat64le", "cfloat64be", "int8",       "uint8",      "bit"};

DataType DataType::parse(std::string_view spec) {
  std::string str(lowercase(spec));

  if (str == "float16")
    return Float16;
  if (str == "float16le")
    return Float16LE;
  if (str == "float16be")
    return Float16BE;

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

  if (str == "cfloat16")
    return CFloat16;
  if (str == "cfloat16le")
    return CFloat16LE;
  if (str == "cfloat16be")
    return CFloat16BE;

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

  throw Exception("invalid data type \"" + spec + "\"");
}

size_t DataType::bits() const {
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
  case Float16:
    return is_complex() ? 32 : 16;
  case Float32:
    return is_complex() ? 64 : 32;
  case Float64:
    return is_complex() ? 128 : 64;
  default:
    throw Exception("invalid datatype specifier");
  }
  return 0;
}

std::string DataType::description() const {
  static const std::string invalid("invalid data type");
  try {
    return dt2str.at(dt).description;
  } catch (std::out_of_range) {
    return invalid;
  }
}

std::string DataType::specifier() const {
  static const std::string invalid("invalid");
  try {
    return dt2str.at(dt).specifier;
  } catch (std::out_of_range) {
    return invalid;
  }
}

DataType DataType::from_command_line(DataType default_datatype) {
  auto opt = App::get_options("datatype");
  if (!opt.empty())
    default_datatype = parse(opt[0][0]);
  return default_datatype;
}

const std::unordered_map<uint8_t, DataType::Strings> DataType::dt2str{
    {Bit, {"Bit", "bitwise"}},
    {Int8, {"Int8", "signed 8 bit integer"}},
    {UInt8, {"UInt8", "unsigned 8 bit integer"}},
    {Int16LE, {"Int16LE", "signed 16 bit integer (little endian)"}},
    {UInt16LE, {"UInt16LE", "unsigned 16 bit integer (little endian)"}},
    {Int16BE, {"Int16BE", "signed 16 bit integer (big endian)"}},
    {UInt16BE, {"UInt16BE", "unsigned 16 bit integer (big endian)"}},
    {Int32LE, {"Int32LE", "signed 32 bit integer (little endian)"}},
    {UInt32LE, {"UInt32LE", "unsigned 32 bit integer (little endian)"}},
    {Int32BE, {"Int32BE", "signed 32 bit integer (big endian)"}},
    {UInt32BE, {"UInt32BE", "unsigned 32 bit integer (big endian)"}},
    {Int64LE, {"Int64LE", "signed 64 bit integer (little endian)"}},
    {UInt64LE, {"UInt64LE", "unsigned 64 bit integer (little endian)"}},
    {Int64BE, {"Int64BE", "signed 64 bit integer (big endian)"}},
    {UInt64BE, {"UInt64BE", "unsigned 64 bit integer (big endian)"}},
    {Float16LE, {"Float16LE", "16 bit float (little endian)"}},
    {Float16BE, {"Float16BE", "16 bit float (big endian)"}},
    {Float32LE, {"Float32LE", "32 bit float (little endian)"}},
    {Float32BE, {"Float32BE", "32 bit float (big endian)"}},
    {Float64LE, {"Float64LE", "64 bit float (little endian)"}},
    {Float64BE, {"Float64BE", "64 bit float (big endian)"}},
    {CFloat16LE, {"CFloat16LE", "Complex 16 bit float (little endian)"}},
    {CFloat16BE, {"CFloat16BE", "Complex 16 bit float (big endian)"}},
    {CFloat32LE, {"CFloat32LE", "Complex 32 bit float (little endian)"}},
    {CFloat32BE, {"CFloat32BE", "Complex 32 bit float (big endian)"}},
    {CFloat64LE, {"CFloat64LE", "Complex 64 bit float (little endian)"}},
    {CFloat64BE, {"CFloat64BE", "Complex 64 bit float (big endian)"}},
    {Undefined, {"Undefined", "undefined"}}};

// clang-format off
App::OptionGroup DataType::options() {
  using namespace App;
  return OptionGroup("Data type options")
         + Option("datatype", "specify output image data type."
                              " Valid choices are: "
                              + join(identifiers, ", ") + ".")
          + Argument("spec").type_choice(identifiers);
}
// clang-format on

} // namespace MR
