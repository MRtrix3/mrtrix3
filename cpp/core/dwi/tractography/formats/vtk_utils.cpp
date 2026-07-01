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

#include "dwi/tractography/formats/vtk_utils.h"

#include <array>
#include <limits>
#include <sstream>
#include <type_traits>
#include <vector>

#include "app.h"
#include "exception.h"
#include "file/mmap.h"
#include "half.h"

namespace MR::DWI::Tractography::Formats::VTKUtils {

void append_file(std::ofstream &dest, const std::filesystem::path &source) {
  std::ifstream in(source, std::ios::binary);
  if (!in)
    throw Exception("unable to open temporary file \"" + source.string() + "\" for VTK assembly");
  dest << in.rdbuf();
  if (!dest.good() && !in.eof())
    throw Exception("error concatenating temporary file \"" + source.string() + "\" into VTK output");
}

void append_bytes(const std::filesystem::path &path, const std::byte *data, size_t size) {
  std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::app);
  if (!out)
    throw Exception("unable to open temporary file \"" + path.string() + "\" for writing");
  out.write(reinterpret_cast<const char *>(data), size);
  if (!out.good())
    throw Exception("error writing to temporary file \"" + path.string() + "\"");
}

std::string dataset_header(Encoding encoding, std::string_view dataset_type) {
  std::string result = "# vtk DataFile Version 3.0\n";

  // Part 2: header line, maximum 256 characters (spec part 2). Prefer the full
  //   command string; fall back to the software name and version if it would not
  //   fit within the permitted header length.
  std::string description = App::command_history_string;
  if (description.empty() || description.size() > header_max_length)
    description = std::string("MRtrix ") + App::mrtrix_version;
  if (description.size() > header_max_length)
    description.resize(header_max_length);
  // The header line is terminated by a newline and may not itself contain one.
  for (char &c : description) {
    if (c == '\n' || c == '\r')
      c = ' ';
  }
  result += description;
  result += '\n';

  result += (encoding == Encoding::ASCII) ? "ASCII\n" : "BINARY\n";
  result += "DATASET ";
  result += dataset_type;
  result += '\n';
  return result;
}

Encoding parse_preamble(std::istream &in, const std::filesystem::path &path, Properties &properties) {
  std::string line;

  // Part 1: version/identifier line.
  if (!std::getline(in, line))
    throw Exception("VTK file \"" + path.string() + "\" is empty");

  // Part 3: ASCII or BINARY. The intervening part-2 header line (a free-text
  //   description) is optional in practice; consume lines until the format
  //   keyword is found, preserving any preceding description as a comment.
  while (std::getline(in, line)) {
    if (line.compare(0, 5, "ASCII") == 0)
      return Encoding::ASCII;
    if (line.compare(0, 6, "BINARY") == 0)
      return Encoding::Binary;
    if (!line.empty()) {
      if (line.back() == '\r')
        line.pop_back();
      if (!line.empty())
        properties.comments.push_back(line);
    }
  }
  throw Exception("VTK file \"" + path.string() + "\" does not declare an ASCII or BINARY format");
}

template <class ValueType> Eigen::Matrix<ValueType, 3, 1> PointReader<ValueType>::get_point(size_t i) const {
  if (encoding == Encoding::ASCII) {
    return {ascii[3 * i], ascii[3 * i + 1], ascii[3 * i + 2]};
  }
  const std::byte *const base = mmap->address();
  if (point_type == PointDataType::Float32) {
    const int64_t offset = binary_offset + static_cast<int64_t>(3 * i * sizeof(float));
    return {static_cast<ValueType>(Raw::fetch_BE<float>(base + offset)),
            static_cast<ValueType>(Raw::fetch_BE<float>(base + offset + sizeof(float))),
            static_cast<ValueType>(Raw::fetch_BE<float>(base + offset + 2 * sizeof(float)))};
  }
  const int64_t offset = binary_offset + static_cast<int64_t>(3 * i * sizeof(double));
  return {static_cast<ValueType>(Raw::fetch_BE<double>(base + offset)),
          static_cast<ValueType>(Raw::fetch_BE<double>(base + offset + sizeof(double))),
          static_cast<ValueType>(Raw::fetch_BE<double>(base + offset + 2 * sizeof(double)))};
}

template <class ValueType>
void write_point(WriteBuffer &buffer, Encoding encoding, const Eigen::Matrix<ValueType, 3, 1> &p) {
  if (encoding == Encoding::ASCII) {
    std::ostringstream stream;
    stream.precision(std::numeric_limits<float>::max_digits10);
    stream << static_cast<float>(p[0]) << " " << static_cast<float>(p[1]) << " " << static_cast<float>(p[2]) << "\n";
    const std::string text = stream.str();
    buffer.add(reinterpret_cast<const std::byte *>(text.data()), text.size());
  } else {
    std::array<float, 3> raw{};
    for (size_t i = 0; i != 3; ++i)
      Raw::store_BE<float>(static_cast<float>(p[i]), raw.data(), i);
    buffer.add(reinterpret_cast<const std::byte *>(raw.data()), sizeof(raw));
  }
}

DataType datatype_from_vtk_token(std::string_view token, const std::filesystem::path &path) {
  // The "bit" token is the on-disk packed boolean; in memory it is carried as a
  //   uint8 (sidecar_value.h, §2.2). Multi-byte types are reported native-endian
  //   so the field round-trips without a precision/endianness detour (D7); the
  //   VTK binary payload is big-endian and is byte-swapped on read/write.
  if (token == "bit" || token == "unsigned_char")
    return DataType(DataType::UInt8);
  if (token == "char")
    return DataType(DataType::Int8);
  if (token == "unsigned_short")
    return DataType::native(DataType(DataType::UInt16));
  if (token == "short")
    return DataType::native(DataType(DataType::Int16));
  if (token == "unsigned_int")
    return DataType::native(DataType(DataType::UInt32));
  if (token == "int")
    return DataType::native(DataType(DataType::Int32));
  if (token == "unsigned_long")
    return DataType::native(DataType(DataType::UInt64));
  if (token == "long")
    return DataType::native(DataType(DataType::Int64));
  if (token == "float")
    return DataType::native(DataType(DataType::Float32));
  if (token == "double")
    return DataType::native(DataType(DataType::Float64));
  throw Exception("VTK file \"" + path.string() + "\" sidecar field datatype \"" + std::string(token) +
                  "\" is unsupported");
}

std::string vtk_token_from_datatype(DataType dtype) {
  switch (dtype() & (DataType::Type | DataType::Signed)) {
  case DataType::Bit:
  case DataType::UInt8:
    return "unsigned_char";
  case DataType::Int8:
    return "char";
  case DataType::UInt16:
    return "unsigned_short";
  case DataType::Int16:
    return "short";
  case DataType::UInt32:
    return "unsigned_int";
  case DataType::Int32:
    return "int";
  case DataType::UInt64:
    return "unsigned_long";
  case DataType::Int64:
    return "long";
  case DataType::Float32:
    return "float";
  case DataType::Float64:
    return "double";
  default:
    throw Exception("sidecar field datatype \"" + dtype.specifier() + "\" has no legacy-VTK representation");
  }
}

namespace {
//! \brief promote a sidecar element to a type that streams as a number (not a char).
template <class T> auto as_printable(T value) {
  if constexpr (std::is_same<T, int8_t>::value || std::is_same<T, uint8_t>::value)
    return static_cast<int>(value);
  else
    return value;
}
} // namespace

template <class T> void write_sidecar_tuple(WriteBuffer &buffer, Encoding encoding, const T *values, size_t M) {
  if (encoding == Encoding::ASCII) {
    std::ostringstream stream;
    if constexpr (std::is_floating_point<T>::value)
      stream.precision(std::numeric_limits<T>::max_digits10);
    for (size_t c = 0; c != M; ++c) {
      if (c != 0)
        stream << " ";
      stream << as_printable(values[c]);
    }
    stream << "\n";
    const std::string text = stream.str();
    buffer.add(reinterpret_cast<const std::byte *>(text.data()), text.size());
  } else {
    std::vector<T> raw(M);
    for (size_t c = 0; c != M; ++c)
      Raw::store_BE<T>(values[c], raw.data(), c);
    buffer.add(reinterpret_cast<const std::byte *>(raw.data()), M * sizeof(T));
  }
}

template <class T> void fetch_sidecar_tuple_BE(const std::byte *base, int64_t offset, T *out, size_t M) {
  for (size_t c = 0; c != M; ++c)
    out[c] = Raw::fetch_BE<T>(base + offset + static_cast<int64_t>(c * sizeof(T)));
}

template class PointReader<float>;
template class PointReader<double>;

template void write_point<float>(WriteBuffer &, Encoding, const Eigen::Matrix<float, 3, 1> &);
template void write_point<double>(WriteBuffer &, Encoding, const Eigen::Matrix<double, 3, 1> &);

#define MRTRIX_VTK_INSTANTIATE_SIDECAR(T)                                                                              \
  template void write_sidecar_tuple<T>(WriteBuffer &, Encoding, const T *, size_t);                                    \
  template void fetch_sidecar_tuple_BE<T>(const std::byte *, int64_t, T *, size_t)
MRTRIX_VTK_INSTANTIATE_SIDECAR(uint8_t);
MRTRIX_VTK_INSTANTIATE_SIDECAR(int8_t);
MRTRIX_VTK_INSTANTIATE_SIDECAR(uint16_t);
MRTRIX_VTK_INSTANTIATE_SIDECAR(int16_t);
MRTRIX_VTK_INSTANTIATE_SIDECAR(uint32_t);
MRTRIX_VTK_INSTANTIATE_SIDECAR(int32_t);
MRTRIX_VTK_INSTANTIATE_SIDECAR(uint64_t);
MRTRIX_VTK_INSTANTIATE_SIDECAR(int64_t);
MRTRIX_VTK_INSTANTIATE_SIDECAR(float);
MRTRIX_VTK_INSTANTIATE_SIDECAR(double);
// Eigen::half completes the DPSValue/DPVValue variant alternative set so the
//   dtype-generic match_v dispatch in the writer links; the legacy VTK format
//   has no float16 token, so such a field is rejected before it reaches here.
MRTRIX_VTK_INSTANTIATE_SIDECAR(Eigen::half);
#undef MRTRIX_VTK_INSTANTIATE_SIDECAR

} // namespace MR::DWI::Tractography::Formats::VTKUtils
