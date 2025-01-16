/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "file/npy.h"

#include <sys/stat.h>

namespace MR::File::NPY {

DataType descr2datatype(const std::string &s) {
  size_t type_offset = 0;
  bool is_little_endian = true;
  bool expect_one_byte_width = false;
  bool issue_endianness_warning = false;
  if (s[0] == '<') {
    type_offset = 1;
  } else if (s[0] == '>') {
    is_little_endian = false;
    type_offset = 1;
  } else if (s[0] == '|') {
    expect_one_byte_width = true;
    type_offset = 1;
  } else {
    issue_endianness_warning = true;
  }
  size_t bytes = 0;
  if (s.size() > type_offset + 1) {
    try {
      bytes = to<size_t>(s.substr(type_offset + 1));
    } catch (Exception &e) {
      throw Exception("Invalid byte width specifier \"" + s.substr(type_offset + 1) + "\"");
    }
  }
  DataType data_type;
  switch (s[type_offset]) {
  case '?':
    data_type = DataType::Bit;
    if (bytes && bytes > 1)
      throw Exception("Unexpected byte width (" + str(bytes) + ") for bitwise data");
    break;
  case 'b':
    data_type = DataType::Int8;
    if (bytes && bytes > 1)
      throw Exception("Unexpected byte width (" + str(bytes) + ") for signed byte data");
    break;
  case 'B':
    data_type = DataType::UInt8;
    if (bytes && bytes > 1)
      throw Exception("Unexpected byte width (" + str(bytes) + ") for unsigned byte data");
    break;
  case 'h':
    data_type = DataType::Int16;
    if (bytes && bytes != 2)
      throw Exception("Unexpected byte width (" + str(bytes) + ") for signed short integer data");
    break;
  case 'H':
    data_type = DataType::UInt16;
    if (bytes && bytes != 2)
      throw Exception("Unexpected byte width (" + str(bytes) + ") for unsigned short integer data");
    break;
  case 'i':
    switch (bytes) {
    case 1:
      data_type = DataType::Int8;
      break;
    case 2:
      data_type = DataType::Int16;
      break;
    case 4:
      data_type = DataType::Int32;
      break;
    case 8:
      data_type = DataType::Int64;
      break;
    default:
      throw Exception("Unexpected bit width (" + str(bytes) + ") for signed integer data");
    }
    break;
  case 'u':
    switch (bytes) {
    case 1:
      data_type = DataType::UInt8;
      break;
    case 2:
      data_type = DataType::UInt16;
      break;
    case 4:
      data_type = DataType::UInt32;
      break;
    case 8:
      data_type = DataType::UInt64;
      break;
    default:
      throw Exception("Unexpected bit width (" + str(bytes) + ") for unsigned integer data");
    }
    break;
  case 'e':
    data_type = DataType::Float16;
    break;
  case 'f':
    switch (bytes) {
    case 2:
      data_type = DataType::Float16;
      break;
    case 4:
      data_type = DataType::Float32;
      break;
    case 8:
      data_type = DataType::Float64;
      break;
    default:
      throw Exception("Unexpected bit width (" + str(bytes) + ") for floating-point data");
    }
    break;
  default:
    throw Exception(std::string("Unsupported data type indicator \'") + s[type_offset] + "\'");
  }
  if (data_type.bytes() != 1 && expect_one_byte_width)
    throw Exception(std::string("Inconsistency in byte width specification") + //
                    " (expected one byte; got " + str(data_type.bytes()) + ")");
  if (bytes > 1) {
    data_type = data_type() | (is_little_endian ? DataType::LittleEndian : DataType::BigEndian);
    if (issue_endianness_warning) {
      using namespace std::string_literals;
      const std::string message = "NumPy file does not indicate data endianness; assuming "s +
                                  (MRTRIX_IS_BIG_ENDIAN ? "big"s : "little"s) + "-endian"s + " (same as system)"s;
      WARN(message);
    }
  }
  return data_type;
}

std::string datatype2descr(const DataType data_type) {
  std::string descr;
  if (data_type.bytes() > 1) {
    if (data_type.is_big_endian())
      descr.push_back('>');
    else if (data_type.is_little_endian())
      descr.push_back('<');
    // Otherwise, use machine endianness, and don't explicitly flag in the file
  }
  if (data_type.is_integer()) {
    descr.push_back(data_type.is_signed() ? 'i' : 'u');
  } else if (data_type.is_floating_point()) {
    // descr.push_back (data_type.is_complex() ? 'c' : 'f');
    if (data_type.is_complex())
      throw Exception("Complex data types not yet supported with NPY format");
    descr.push_back('f');
  } else if (data_type == DataType::Bit) {
    descr.push_back('?');
    return descr;
  }
  descr.append(str(data_type.bytes()));
  return descr;
}

// CONF option: NPYFloatMaxSavePrecision
// CONF default: 64
// CONF When exporting floating-point data to NumPy .npy format, do not
// CONF use a precision any greater than this value in bits (used to
// CONF minimise file size). Must be equal to either 16, 32 or 64.
size_t float_max_save_precision() {
  static size_t result = to<size_t>(File::Config::get("NPYFloatMaxSavePrecision", "64"));
  if (!(result == 16 || result == 32 || result == 64))
    throw Exception("Invalid value for config file entry \"NPYFloatMaxSavePrecision\""
                    " (must be 16, 32 or 64)");
  return result;
}

KeyValues parse_dict(std::string s) {
  if (s[s.size() - 1] == '\n')
    s.resize(s.size() - 1);
  // Remove trailing space-padding
  s = strip(s, " ", false, true);
  // Expect a dictionary literal; remove curly braces
  s = strip(strip(s, "{", true, false), "}", false, true);
  const std::map<char, char> pairs{{'{', '}'}, {'[', ']'}, {'(', ')'}, {'\'', '\''}, {'"', '"'}};
  std::vector<char> openers;
  bool prev_was_escape = false;
  std::string current, key;
  KeyValues keyval;
  for (const auto c : s) {
    // std::cerr << "Openers: [" << join(openers, ",") << "]; prev_was_escape = " << str(prev_was_escape) << "; current:
    // " << current << "; key: " << key << "\n";
    if (prev_was_escape) {
      current.push_back(c);
      prev_was_escape = false;
      continue;
    }
    if (c == ' ' && current.empty())
      continue;
    if (c == '\\') {
      if (prev_was_escape) {
        current.push_back('\\');
        prev_was_escape = false;
      } else {
        prev_was_escape = true;
      }
      continue;
    }
    if (!openers.empty()) {
      if (c == pairs.find(openers.back())->second)
        openers.pop_back();
      // If final opener is a string quote, and it's now being closed,
      //   want to capture anything until the corresponding close,
      //   regardless of whether or not it is itself an opener
      // For other openers, a new opener can stack, and so we
      //   don't want to immediately call continue
      if (c == '\'' || c == '\"') {
        current.push_back(c);
        continue;
      }
    } else if (c == ':') {
      if (!key.empty())
        throw Exception("Error parsing NumPy header:"
                        " non-isolated colon separator");
      if ((current.front() == '\"' && current.back() == '\"') || (current.front() == '\'' && current.back() == '\''))
        key = current.substr(1, current.size() - 2);
      else
        key = current;
      if (keyval.find(key) != keyval.end())
        throw Exception("Error parsing NumPy header:"
                        " duplicate key");
      current.clear();
      continue;
    } else if (c == ',') {
      if (key.empty())
        throw Exception("Error parsing NumPy header:"
                        " colon separator with no colon-separated key beforehand");
      if ((current.front() == '\"' && current.back() == '\"') || (current.front() == '\'' && current.back() == '\''))
        current = current.substr(1, current.size() - 2);
      keyval[key] = current;
      key.clear();
      current.clear();
      continue;
    }
    if (pairs.find(c) != pairs.end())
      openers.push_back(c);
    current.push_back(c);
  }
  if (!openers.empty()) {
    throw Exception("Error parsing NumPy header:"
                    " unpaired bracket or quotation symbol(s) at EOF");
  }
  if (!key.empty()) {
    throw Exception("Error parsing NumPy header:"
                    " key without associated value at EOF");
  }
  current = strip(current, " ,");
  if (!current.empty()) {
    throw Exception("Error parsing NumPy header:"
                    " non-empty content at EOF");
  }

  // std::cerr << "Final keyvalues: {";
  // for (const auto& kv : keyval)
  //   std::cerr << " '" + kv.first + "': '" + kv.second + "', ";
  // std::cerr << "}\n";
  return keyval;
}

ReadInfo read_header(const std::string &path) {
  ReadInfo info;
  std::ifstream in(path, std::ios_base::in | std::ios_base::binary);
  if (!in)
    throw Exception("Unable to load file \"" + path + "\"");

  unsigned char magic[7];
  in.read(reinterpret_cast<char *>(magic), 6);
  if (memcmp(magic, magic_string, 6)) {
    magic[6] = 0;
    throw Exception("Invalid magic string in NPY binary file \"" + path + "\": " + str(magic));
  }
  uint8_t major_version, minor_version;
  in.read(reinterpret_cast<char *>(&major_version), 1);
  in.read(reinterpret_cast<char *>(&minor_version), 1);
  uint32_t header_len;
  switch (major_version) {
  case 1:
    uint16_t header_len_short;
    in.read(reinterpret_cast<char *>(&header_len_short), 2);
    // header length always stored on filesystem as little-endian
    header_len = uint32_t(ByteOrder::LE(header_len_short));
    break;
  case 2:
    in.read(reinterpret_cast<char *>(&header_len), 4);
    // header length always stored on filesystem as little-endian
    header_len = ByteOrder::LE(header_len);
    break;
  default:
    throw Exception("Incompatible major version (" + str(major_version) + ") detected in NumPy file \"" + path + "\"");
  }
  std::unique_ptr<char[]> header_cstr(new char[header_len + 1]);
  in.read(header_cstr.get(), header_len);
  header_cstr[header_len] = '\0';
  info.data_offset = in.tellg();
  in.close();

  try {
    info.keyval = parse_dict(std::string(header_cstr.get()));
  } catch (Exception &e) {
    throw Exception(e, "Error parsing header of NumPy file \"" + path + "\"");
  }
  const auto descr_ptr = info.keyval.find("descr");
  if (descr_ptr == info.keyval.end())
    throw Exception("Error parsing header of NumPy file \"" + path + "\": \"descr\" key absent");
  const std::string descr = descr_ptr->second;
  info.keyval.erase(descr_ptr);
  try {
    info.data_type = descr2datatype(descr);
  } catch (Exception &e) {
    throw Exception(e, "Error parsing header of NumPy file \"" + path + "\"");
  }
  const auto fortran_order_ptr = info.keyval.find("fortran_order");
  if (fortran_order_ptr == info.keyval.end())
    throw Exception("Error parsing header of NumPy file \"" + path + "\": \"fortran_order\" key absent");
  info.column_major = to<bool>(fortran_order_ptr->second);
  info.keyval.erase(fortran_order_ptr);
  const auto shape_ptr = info.keyval.find("shape");
  if (shape_ptr == info.keyval.end())
    throw Exception("Error parsing header of NumPy file \"" + path + "\": \"shape\" key absent");
  const std::string shape_str = shape_ptr->second;
  info.keyval.erase(shape_ptr);
  // Strip the brackets and split by commas
  auto shape_split_str = split(strip(strip(shape_str, "(", true, false), ")", false, true), ",", true);
  if (shape_split_str.size() > 2)
    throw Exception("NumPy file \"" + path + "\" contains more than two dimensions: " + shape_str);
  for (const auto &s : shape_split_str)
    info.shape.push_back(to<ssize_t>(s));

  // Make sure that the size of the file matches expectations given the offset to the data, the shape, and the data type
  struct stat sbuf;
  if (stat(path.c_str(), &sbuf))
    throw Exception("Cannot query size of NumPy file \"" + path + "\": " + strerror(errno));
  const size_t file_size = sbuf.st_size;
  size_t num_elements = info.shape[0];
  if (info.shape.size() == 2)
    num_elements *= info.shape[1];
  const size_t predicted_data_size = num_elements * info.data_type.bytes();
  if (info.data_offset + predicted_data_size != file_size)
    throw Exception("Size of NumPy file \"" + path + "\" (" + str(file_size) + ") does not meet expectations given " +
                    "total header size (" + str(info.data_offset) + ") " + "and predicted data size (" + "(" +
                    str(info.shape[0]) + (info.shape.size() == 2 ? "x" + str(info.shape[1]) : "") + " = " +
                    str(num_elements) + ") " + "values x " + str(info.data_type.bytes()) +
                    " bytes per value = " + str(num_elements * info.data_type.bytes()) + " bytes)");

  return info;
}

WriteInfo prepare_ND_write(const std::string &path, const DataType data_type, const std::vector<size_t> &shape) {
  assert(shape.size() == 1 || shape.size() == 2);
  WriteInfo info;
  info.data_type = data_type;
  if (info.data_type.is_floating_point()) {
    if (info.data_type.is_complex())
      throw Exception("Complex data types not yet supported with NPY format");
    const size_t max_precision = float_max_save_precision();
    if (max_precision < info.data_type.bits()) {
      INFO("Precision of floating-point NumPy file \"" + path + "\" decreased from native " +
           str(info.data_type.bits()) + " bits to " + str(max_precision));
      if (max_precision == 16)
        info.data_type = DataType::native(DataType::Float16);
      else
        info.data_type = DataType::native(DataType::Float32);
    }
  }

  // Need to construct the header string in order to discover its length
  std::string header(std::string("{'descr': '") + datatype2descr(info.data_type) +
                     "', 'fortran_order': " + (shape.size() == 2 ? "True" : "False") + ", 'shape': (" + str(shape[0]) +
                     "," + (shape.size() == 2 ? (" " + str(shape[1])) : "") + "), }");
  // Pad with spaces so that, for version 1, upon adding a newline at the end, the file size (i.e. eventual offset to
  // the data) is a multiple of alignment (16)
  uint32_t space_count =
      alignment -
      ((header.size() + 11) % alignment); // 11 = 6 magic number + 2 version + 2 header length + 1 newline for header
  uint32_t padded_header_length = header.size() + space_count + 1;
  File::OFStream out(path, std::ios_base::out | std::ios_base::binary);
  if (!out)
    throw Exception("Unable to create NumPy file \"" + path + "\"");
  out.write(reinterpret_cast<const char *>(magic_string), 6);
  const unsigned char minor_version = '\x00';
  if (10 + padded_header_length > std::numeric_limits<uint16_t>::max()) {
    const unsigned char major_version = '\x02';
    out.write(reinterpret_cast<const char *>(&major_version), 1);
    out.write(reinterpret_cast<const char *>(&minor_version), 1);
    space_count = alignment - ((header.size() + 13) %
                               alignment); // 13 = 6 magic number + 2 version + 4 header length + 1 newline for header
    padded_header_length = header.size() + space_count + 1;
    padded_header_length = ByteOrder::LE(padded_header_length);
    out.write(reinterpret_cast<const char *>(&padded_header_length), 4);
  } else {
    const unsigned char major_version = '\x01';
    out.write(reinterpret_cast<const char *>(&major_version), 1);
    out.write(reinterpret_cast<const char *>(&minor_version), 1);
    uint16_t padded_header_length_short = ByteOrder::LE(uint16_t(padded_header_length));
    out.write(reinterpret_cast<const char *>(&padded_header_length_short), 2);
  }
  for (size_t i = 0; i != space_count; ++i)
    header.push_back(' ');
  header.push_back('\n');
  out.write(header.c_str(), header.size());
  const int64_t leadin_size = out.tellp();
  assert(!(out.tellp() % alignment));
  out.close();
  const size_t num_elements = shape[0] * (shape.size() == 2 ? shape[1] : 1);
  const size_t data_size = num_elements * info.data_type.bytes();
  File::resize(path, leadin_size + data_size);
  info.mmap.reset(new File::MMap({path, leadin_size}, true, false));
  return info;
}

} // namespace MR::File::NPY
