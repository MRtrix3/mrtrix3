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

#include "dwi/tractography/formats/vtk.h"

#include <array>
#include <cctype>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>

#include "app.h"
#include "exception.h"
#include "file/config.h"
#include "file/entry.h"
#include "file/mmap.h"
#include "file/ofstream.h"
#include "file/temp.h"
#include "raw.h"

namespace MR::DWI::Tractography {

namespace {

//! \brief Maximum length of the VTK header line (part 2 of the spec).
constexpr size_t vtk_header_max_length = 256;

//! \brief Append the contents of \a source to the already-open stream \a dest.
void append_file(std::ofstream &dest, const std::filesystem::path &source) {
  std::ifstream in(source, std::ios::binary);
  if (!in)
    throw Exception("unable to open temporary file \"" + source.string() + "\" for VTK assembly");
  dest << in.rdbuf();
  if (!dest.good() && !in.eof())
    throw Exception("error concatenating temporary file \"" + source.string() + "\" into VTK output");
}

//! \brief Append \a size bytes of \a data to the file at \a path (binary append).
void append_bytes(const std::filesystem::path &path, const std::byte *data, size_t size) {
  std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::app);
  if (!out)
    throw Exception("unable to open temporary file \"" + path.string() + "\" for writing");
  out.write(reinterpret_cast<const char *>(data), size);
  if (!out.good())
    throw Exception("error writing to temporary file \"" + path.string() + "\"");
}

} // namespace

/* ************************************************************************ */
/*                          VTKReader<ValueType>                           */
/* ************************************************************************ */

template <class ValueType>
VTKReader<ValueType>::VTKReader(const std::filesystem::path &path, Properties &properties)
    : points_offset(0), num_points(0), current_streamline(0), current_vertex(0), current_index(0) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    throw Exception("unable to open VTK file \"" + path.string() + "\"");

  std::string line;

  // Part 1: version/identifier line.
  if (!std::getline(in, line))
    throw Exception("VTK file \"" + path.string() + "\" is empty");

  // Part 3: ASCII or BINARY. The intervening part-2 header line (a free-text
  //   description) is optional in practice; consume lines until the format
  //   keyword is found, preserving any preceding description as a comment.
  bool encoding_found = false;
  while (std::getline(in, line)) {
    if (line.compare(0, 5, "ASCII") == 0) {
      encoding = Encoding::ASCII;
      encoding_found = true;
      break;
    }
    if (line.compare(0, 6, "BINARY") == 0) {
      encoding = Encoding::Binary;
      encoding_found = true;
      break;
    }
    if (!line.empty()) {
      if (line.back() == '\r')
        line.pop_back();
      if (!line.empty())
        properties.comments.push_back(line);
    }
  }
  if (!encoding_found)
    throw Exception("VTK file \"" + path.string() + "\" does not declare an ASCII or BINARY format");

  // Part 4: dataset structure; only POLYDATA with POINTS and LINES is admitted.
  bool have_points = false;
  bool have_lines = false;
  int64_t lines_offset = 0;
  size_t num_lines = 0;
  size_t lines_list_size = 0;
  bool lines_int64 = false;

  while (std::getline(in, line)) {
    std::string keyword;
    {
      std::istringstream stream(line);
      stream >> keyword;
    }
    if (keyword.empty())
      continue;

    if (keyword == "DATASET") {
      std::istringstream stream(line);
      std::string dummy;
      std::string type;
      stream >> dummy >> type;
      if (type != "POLYDATA")
        throw Exception("VTK file \"" + path.string() + "\" is not POLYDATA (DATASET " + type +
                        "); only POINTS/LINES POLYDATA is supported");
      continue;
    }

    if (keyword == "POINTS") {
      std::istringstream stream(line);
      std::string dummy;
      std::string datatype;
      stream >> dummy >> num_points >> datatype;
      if (datatype == "float")
        point_type = PointType::Float32;
      else if (datatype == "double")
        point_type = PointType::Float64;
      else
        throw Exception("VTK file \"" + path.string() + "\" POINTS datatype \"" + datatype +
                        "\" is unsupported (expected float or double)");
      have_points = true;
      const size_t element_size = (point_type == PointType::Float32) ? sizeof(float) : sizeof(double);

      if (encoding == Encoding::Binary) {
        points_offset = static_cast<int64_t>(in.tellg());
        // Skip the POINTS payload to reach the next keyword.
        in.seekg(points_offset + static_cast<int64_t>(3 * num_points * element_size), std::ios::beg);
      } else {
        ascii_points.resize(3 * num_points);
        for (size_t i = 0; i != 3 * num_points; ++i) {
          double value = 0.0;
          if (!(in >> value))
            throw Exception("VTK file \"" + path.string() + "\" truncated while reading ASCII POINTS data");
          ascii_points[i] = static_cast<ValueType>(value);
        }
      }
      continue;
    }

    if (keyword == "LINES") {
      std::istringstream stream(line);
      std::string dummy;
      stream >> dummy >> num_lines >> lines_list_size;
      // A "vtktypeint64" qualifier (emitted by newer VTK) selects 64-bit indices.
      lines_int64 = line.find("vtktypeint64") != std::string::npos;
      lines_offset = static_cast<int64_t>(in.tellg());
      have_lines = true;
      // The LINES block is the last field of interest; stop scanning the header.
      break;
    }

    // Any other dataset field (VERTICES / POLYGONS / TRIANGLE_STRIPS / POINT_DATA
    //   / CELL_DATA / SCALARS / etc.) is not part of a tractogram representation.
    throw Exception("VTK file \"" + path.string() + "\" contains unsupported dataset field \"" + keyword +
                    "\"; only POINTS and LINES are permitted");
  }

  if (!have_points)
    throw Exception("VTK file \"" + path.string() + "\" contains no POINTS data");

  // A file with no LINES contains no streamlines; this is a valid empty tractogram.
  if (!have_lines)
    num_lines = 0;

  // Scan the LINES block up-front: verify sequential vertex ordering and record
  //   the per-streamline vertex count.
  streamline_sizes.reserve(num_lines);
  if (have_lines) {
    if (encoding == Encoding::Binary) {
      mmap.reset(new File::MMap(File::Entry(path), false, true));
      const std::byte *const base = mmap->address();
      const int64_t mapped_size = mmap->size();
      const size_t index_size = lines_int64 ? sizeof(int64_t) : sizeof(int32_t);
      int64_t pos = lines_offset;
      size_t running_vertex = 0;
      for (size_t l = 0; l != num_lines; ++l) {
        if (pos + static_cast<int64_t>(index_size) > mapped_size)
          throw Exception("VTK file \"" + path.string() + "\" truncated within LINES block");
        const int64_t count = lines_int64 ? Raw::fetch_BE<int64_t>(base + pos) : Raw::fetch_BE<int32_t>(base + pos);
        pos += static_cast<int64_t>(index_size);
        if (count < 0)
          throw Exception("VTK file \"" + path.string() + "\" contains a negative LINES vertex count");
        if (pos + count * static_cast<int64_t>(index_size) > mapped_size)
          throw Exception("VTK file \"" + path.string() + "\" truncated within LINES block");
        for (int64_t v = 0; v != count; ++v) {
          const int64_t idx = lines_int64 ? Raw::fetch_BE<int64_t>(base + pos) : Raw::fetch_BE<int32_t>(base + pos);
          pos += static_cast<int64_t>(index_size);
          if (idx != static_cast<int64_t>(running_vertex + static_cast<size_t>(v)))
            throw Exception("VTK file \"" + path.string() +
                            "\" LINES indices are not a sequential vertex run; "
                            "only sequentially-ordered streamlines are supported");
        }
        running_vertex += static_cast<size_t>(count);
        streamline_sizes.push_back(static_cast<uint32_t>(count));
      }
    } else {
      // ASCII LINES connectivity follows the header on the same input stream.
      size_t running_vertex = 0;
      for (size_t l = 0; l != num_lines; ++l) {
        int64_t count = 0;
        if (!(in >> count))
          throw Exception("VTK file \"" + path.string() + "\" truncated within ASCII LINES block");
        if (count < 0)
          throw Exception("VTK file \"" + path.string() + "\" contains a negative LINES vertex count");
        for (int64_t v = 0; v != count; ++v) {
          int64_t idx = 0;
          if (!(in >> idx))
            throw Exception("VTK file \"" + path.string() + "\" truncated within ASCII LINES block");
          if (idx != static_cast<int64_t>(running_vertex + static_cast<size_t>(v)))
            throw Exception("VTK file \"" + path.string() +
                            "\" LINES indices are not a sequential vertex run; "
                            "only sequentially-ordered streamlines are supported");
        }
        running_vertex += static_cast<size_t>(count);
        streamline_sizes.push_back(static_cast<uint32_t>(count));
      }
    }
  }

  // For binary POINTS access, establish the memory-map if not already created.
  if (encoding == Encoding::Binary && mmap == nullptr)
    mmap.reset(new File::MMap(File::Entry(path), false, true));
}

template <class ValueType> VTKReader<ValueType>::~VTKReader() = default;

template <class ValueType> Eigen::Matrix<ValueType, 3, 1> VTKReader<ValueType>::get_point(size_t i) const {
  if (encoding == Encoding::ASCII) {
    return {ascii_points[3 * i], ascii_points[3 * i + 1], ascii_points[3 * i + 2]};
  }
  const std::byte *const base = mmap->address();
  if (point_type == PointType::Float32) {
    const int64_t offset = points_offset + static_cast<int64_t>(3 * i * sizeof(float));
    return {static_cast<ValueType>(Raw::fetch_BE<float>(base + offset)),
            static_cast<ValueType>(Raw::fetch_BE<float>(base + offset + sizeof(float))),
            static_cast<ValueType>(Raw::fetch_BE<float>(base + offset + 2 * sizeof(float)))};
  }
  const int64_t offset = points_offset + static_cast<int64_t>(3 * i * sizeof(double));
  return {static_cast<ValueType>(Raw::fetch_BE<double>(base + offset)),
          static_cast<ValueType>(Raw::fetch_BE<double>(base + offset + sizeof(double))),
          static_cast<ValueType>(Raw::fetch_BE<double>(base + offset + 2 * sizeof(double)))};
}

template <class ValueType> bool VTKReader<ValueType>::operator()(Streamline<ValueType> &tck) {
  tck.clear();
  if (current_streamline >= streamline_sizes.size())
    return false;
  const uint32_t count = streamline_sizes[current_streamline++];
  tck.set_index(current_index++);
  for (uint32_t v = 0; v != count; ++v)
    tck.push_back(get_point(current_vertex++));
  return true;
}

/* ************************************************************************ */
/*                          VTKWriter<ValueType>                           */
/* ************************************************************************ */

template <class ValueType>
VTKWriter<ValueType>::VTKWriter(const std::filesystem::path &path, const Properties &properties)
    : path(path),
      points_buffer(File::Config::get_int("TrackWriterBufferSize", 16777216), 1),
      lines_buffer(File::Config::get_int("TrackWriterBufferSize", 16777216), 1),
      num_points(0),
      num_lines(0),
      lines_list_size(0) {
  if (path.extension() != ".vtk")
    throw Exception("output VTK track files must use the .vtk suffix");

  // The "-ascii" option (declared by commands that write VTK, e.g. tckconvert)
  //   selects the ASCII encoding; the BINARY (big-endian) encoding is the default.
  encoding = App::get_options("ascii").empty() ? Encoding::Binary : Encoding::ASCII;

  (void)properties;

  // Verify the output can be created, then truncate any pre-existing content.
  App::check_overwrite(path);
  {
    File::OFStream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out.good())
      throw Exception("unable to create VTK output file \"" + path.string() + "\"");
  }

  points_tempfile = File::create_tempfile(0, ".vtkpoints");
  lines_tempfile = File::create_tempfile(0, ".vtklines");

  const std::filesystem::path points_path = points_tempfile;
  const std::filesystem::path lines_path = lines_tempfile;
  points_buffer.set_flush_callback(
      [points_path](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &) {
        append_bytes(points_path, data, size);
      });
  lines_buffer.set_flush_callback(
      [lines_path](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &) {
        append_bytes(lines_path, data, size);
      });
}

template <class ValueType> std::string VTKWriter<ValueType>::header() const {
  std::string result = "# vtk DataFile Version 3.0\n";

  // Part 2: header line, maximum 256 characters (spec part 2). Prefer the full
  //   command string; fall back to the software name and version if it would not
  //   fit within the permitted header length.
  std::string description = App::command_history_string;
  if (description.empty() || description.size() > vtk_header_max_length)
    description = std::string("MRtrix ") + App::mrtrix_version;
  if (description.size() > vtk_header_max_length)
    description.resize(vtk_header_max_length);
  // The header line is terminated by a newline and may not itself contain one.
  for (char &c : description) {
    if (c == '\n' || c == '\r')
      c = ' ';
  }
  result += description;
  result += '\n';

  result += (encoding == Encoding::ASCII) ? "ASCII\n" : "BINARY\n";
  result += "DATASET POLYDATA\n";
  return result;
}

template <class ValueType> void VTKWriter<ValueType>::add_point(const Eigen::Matrix<ValueType, 3, 1> &p) {
  if (encoding == Encoding::ASCII) {
    std::ostringstream stream;
    // Coordinates are stored as float32 (matching the binary encoding); print
    //   enough significant digits to round-trip float32 losslessly.
    stream.precision(std::numeric_limits<float>::max_digits10);
    stream << static_cast<float>(p[0]) << " " << static_cast<float>(p[1]) << " " << static_cast<float>(p[2]) << "\n";
    const std::string text = stream.str();
    points_buffer.add(reinterpret_cast<const std::byte *>(text.data()), text.size());
  } else {
    std::array<float, 3> raw{};
    for (size_t i = 0; i != 3; ++i)
      Raw::store_BE<float>(static_cast<float>(p[i]), raw.data(), i);
    points_buffer.add(reinterpret_cast<const std::byte *>(raw.data()), sizeof(raw));
  }
}

template <class ValueType> void VTKWriter<ValueType>::add_line(size_t first_vertex, size_t num_vertices) {
  if (encoding == Encoding::ASCII) {
    std::ostringstream stream;
    stream << num_vertices;
    for (size_t i = 0; i != num_vertices; ++i)
      stream << " " << (first_vertex + i);
    stream << "\n";
    const std::string text = stream.str();
    lines_buffer.add(reinterpret_cast<const std::byte *>(text.data()), text.size());
  } else {
    std::vector<int32_t> raw(num_vertices + 1);
    Raw::store_BE<int32_t>(static_cast<int32_t>(num_vertices), raw.data(), 0);
    for (size_t i = 0; i != num_vertices; ++i)
      Raw::store_BE<int32_t>(static_cast<int32_t>(first_vertex + i), raw.data(), i + 1);
    lines_buffer.add(reinterpret_cast<const std::byte *>(raw.data()), raw.size() * sizeof(int32_t));
  }
}

template <class ValueType> bool VTKWriter<ValueType>::operator()(const Streamline<ValueType> &tck) {
  const size_t first_vertex = num_points;
  for (const auto &pos : tck)
    add_point(pos);
  num_points += tck.size();
  add_line(first_vertex, tck.size());
  ++num_lines;
  // The LINES connectivity list holds, per streamline, the count plus one index
  //   per vertex.
  lines_list_size += tck.size() + 1;
  return true;
}

template <class ValueType> void VTKWriter<ValueType>::finalise() {
  points_buffer.commit();
  lines_buffer.commit();

  File::OFStream out(path, std::ios::out | std::ios::binary | std::ios::trunc);

  out << header();
  out << "POINTS " << num_points << " float\n";
  append_file(out, points_tempfile);
  // Binary payloads must be separated from the following ASCII keyword by a
  //   newline; ASCII payloads already end each record with one.
  if (encoding == Encoding::Binary)
    out << "\n";

  out << "LINES " << num_lines << " " << lines_list_size << "\n";
  append_file(out, lines_tempfile);
  if (encoding == Encoding::Binary)
    out << "\n";

  out.close();

  std::error_code ec;
  std::filesystem::remove(points_tempfile, ec);
  std::filesystem::remove(lines_tempfile, ec);
}

template <class ValueType> VTKWriter<ValueType>::~VTKWriter() {
  try {
    finalise();
  } catch (Exception &e) {
    Exception(e, "VTK tractography file not properly finalised").display();
  }
}

/* ************************************************************************ */
/*               Explicit instantiation for float and double              */
/* ************************************************************************ */

template class VTKReader<float>;
template class VTKReader<double>;
template class VTKWriter<float>;
template class VTKWriter<double>;

namespace Formats {

bool VTK::handles(const std::filesystem::path &path) const { return path.extension() == ".vtk"; }

std::unique_ptr<ReaderInterface<float>> VTK::read_float(const std::filesystem::path &path,
                                                        Properties &properties) const {
  return std::make_unique<VTKReader<float>>(path, properties);
}

std::unique_ptr<ReaderInterface<double>> VTK::read_double(const std::filesystem::path &path,
                                                          Properties &properties) const {
  return std::make_unique<VTKReader<double>>(path, properties);
}

std::unique_ptr<WriterInterface<float>> VTK::create_float(const std::filesystem::path &path,
                                                          const Properties &properties) const {
  return std::make_unique<VTKWriter<float>>(path, properties);
}

std::unique_ptr<WriterInterface<double>> VTK::create_double(const std::filesystem::path &path,
                                                            const Properties &properties) const {
  return std::make_unique<VTKWriter<double>>(path, properties);
}

} // namespace Formats

} // namespace MR::DWI::Tractography
