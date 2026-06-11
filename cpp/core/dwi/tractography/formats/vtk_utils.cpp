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

#include "app.h"
#include "exception.h"
#include "file/mmap.h"

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

template class PointReader<float>;
template class PointReader<double>;

template void write_point<float>(WriteBuffer &, Encoding, const Eigen::Matrix<float, 3, 1> &);
template void write_point<double>(WriteBuffer &, Encoding, const Eigen::Matrix<double, 3, 1> &);

} // namespace MR::DWI::Tractography::Formats::VTKUtils
