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

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "app.h"
#include "exception.h"
#include "file/config.h"
#include "file/entry.h"
#include "file/mmap.h"
#include "file/ofstream.h"
#include "file/temp.h"
#include "match_variant.h"
#include "raw.h"

#include "dwi/tractography/formats/vtk_utils.h"
#include "dwi/tractography/sidecar_value.h"

namespace MR::DWI::Tractography {

namespace VTKUtils = Formats::VTKUtils;
using VTKUtils::Encoding;
using VTKUtils::PointDataType;

/* ************************************************************************ */
/*                          VTKReader<ValueType>                           */
/* ************************************************************************ */

template <class ValueType>
VTKReader<ValueType>::VTKReader(const std::filesystem::path &path, Properties &properties)
    : current_streamline(0), current_vertex(0), current_index(0) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    throw Exception("unable to open VTK file \"" + path.string() + "\"");

  std::string line;

  // Parts 1-3: version line, optional description comments, and the ASCII/BINARY
  //   encoding keyword (shared with the ".vtx" reader; see VTKUtils).
  encoding = VTKUtils::parse_preamble(in, path, properties);

  // Part 4: dataset structure; only POLYDATA with POINTS and LINES is admitted.
  bool have_points = false;
  bool have_lines = false;
  int64_t lines_offset = 0;
  size_t num_lines = 0;
  size_t lines_list_size = 0;
  bool lines_int64 = false;
  size_t num_points = 0;
  PointDataType point_type = PointDataType::Float32;
  int64_t points_offset = 0;
  std::vector<ValueType> ascii_points;

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
        point_type = PointDataType::Float32;
      else if (datatype == "double")
        point_type = PointDataType::Float64;
      else
        throw Exception("VTK file \"" + path.string() + "\" POINTS datatype \"" + datatype +
                        "\" is unsupported (expected float or double)");
      have_points = true;
      const size_t element_size = (point_type == PointDataType::Float32) ? sizeof(float) : sizeof(double);

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

  // Construct the shared POINTS accessor (VTKUtils): ASCII coordinates already in
  //   RAM, or a binary big-endian array over the memory-map (the LINES scan above
  //   established the map; create it here if there were no streamlines to scan).
  if (encoding == Encoding::Binary) {
    if (mmap == nullptr)
      mmap = std::make_shared<File::MMap>(File::Entry(path), false, true);
    points = std::make_unique<VTKUtils::PointReader<ValueType>>(mmap, points_offset, point_type, num_points);
  } else {
    points = std::make_unique<VTKUtils::PointReader<ValueType>>(std::move(ascii_points), num_points);
  }
}

template <class ValueType> VTKReader<ValueType>::~VTKReader() = default;

template <class ValueType> bool VTKReader<ValueType>::operator()(Streamline<ValueType> &tck) {
  tck.clear();
  if (current_streamline >= streamline_sizes.size())
    return false;
  const uint32_t count = streamline_sizes[current_streamline++];
  tck.set_index(current_index++);
  for (uint32_t v = 0; v != count; ++v)
    tck.push_back(points->get_point(current_vertex++));
  return true;
}

/* ************************************************************************ */
/*                          VTKWriter<ValueType>                           */
/* ************************************************************************ */

template <class ValueType>
VTKWriter<ValueType>::VTKWriter(const std::filesystem::path &path,
                                const Properties &properties,
                                const FieldRegistry &registry)
    : path(path),
      registry(registry),
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
        VTKUtils::append_bytes(points_path, data, size);
      });
  lines_buffer.set_flush_callback(
      [lines_path](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &) {
        VTKUtils::append_bytes(lines_path, data, size);
      });

  // One own temporary file + WriteBuffer per sidecar field (Stage 13 step 3):
  //   every dps/dpv field is streamed independently and concatenated behind its
  //   POINT_DATA / CELL_DATA attribute header on finalisation. The reserved
  //   "weight" field is sourced from streamline.weight, not from the dps vector,
  //   and is not a generic VTK attribute, so it is skipped here.
  const size_t buffer_size = File::Config::get_int("TrackWriterBufferSize", 16777216);
  for (const auto &field : registry) {
    if (field.role != FieldRole::DPS && field.role != FieldRole::DPV)
      continue;
    SidecarOutput output;
    output.descriptor = field;
    output.tempfile = File::create_tempfile(0, ".vtksidecar");
    output.buffer = std::make_shared<Formats::WriteBuffer>(buffer_size, 1);
    const std::filesystem::path field_path = output.tempfile;
    output.buffer->set_flush_callback(
        [field_path](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &) {
          VTKUtils::append_bytes(field_path, data, size);
        });
    sidecars.push_back(std::move(output));
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
    VTKUtils::write_point<ValueType>(points_buffer, encoding, pos);
  num_points += tck.size();
  add_line(first_vertex, tck.size());
  ++num_lines;
  // The LINES connectivity list holds, per streamline, the count plus one index
  //   per vertex.
  lines_list_size += tck.size() + 1;
  return true;
}

template <class ValueType> bool VTKWriter<ValueType>::operator()(const TractogramItem<ValueType> &item) {
  // The vertices-and-topology write is identical to the bare-streamline path;
  //   the sidecar values are streamed in lock-step to their own temporary files.
  (*this)(item.streamline);
  add_sidecars(item);
  return true;
}

template <class ValueType> void VTKWriter<ValueType>::add_sidecars(const TractogramItem<ValueType> &item) {
  for (auto &field : sidecars) {
    const FieldDescriptor &descriptor = field.descriptor;
    if (descriptor.role == FieldRole::DPS) {
      assert(descriptor.ordinal < item.dps.size());
      // One M-component tuple for this streamline (D3: scalar when M==1).
      MR::match_v(item.dps[descriptor.ordinal], [&](const auto &value) {
        using Element = typename std::decay_t<decltype(value)>::element_type;
        VTKUtils::write_sidecar_tuple<Element>(*field.buffer, encoding, value.data(), descriptor.columns);
      });
    } else {
      assert(descriptor.ordinal < item.dpv.size());
      // One M-component tuple per vertex; serialise row-major (per-vertex
      //   contiguous) so the reader can step one vertex at a time.
      MR::match_v(item.dpv[descriptor.ordinal], [&](const auto &value) {
        using Element = typename std::decay_t<decltype(value)>::element_type;
        const Eigen::Index rows = value.rows();
        std::vector<Element> row(descriptor.columns);
        for (Eigen::Index r = 0; r != rows; ++r) {
          for (size_t c = 0; c != descriptor.columns; ++c)
            row[c] = value(r, static_cast<Eigen::Index>(c));
          VTKUtils::write_sidecar_tuple<Element>(*field.buffer, encoding, row.data(), descriptor.columns);
        }
      });
    }
  }
}

template <class ValueType> void VTKWriter<ValueType>::append_sidecar(std::ofstream &out, const SidecarOutput &field) {
  const FieldDescriptor &descriptor = field.descriptor;
  const std::string token = VTKUtils::vtk_token_from_datatype(descriptor.dtype);
  // SCALARS supports 1..4 components and an arbitrary dataType (incl.
  //   unsigned_char, so native uint8 RGB round-trips losslessly through both
  //   encodings); wider fields use FIELD, the general multi-column carrier.
  if (descriptor.columns >= 1 && descriptor.columns <= 4) {
    out << "SCALARS " << descriptor.name << " " << token << " " << descriptor.columns << "\n";
    out << "LOOKUP_TABLE default\n";
  } else {
    const size_t num_tuples = (descriptor.role == FieldRole::DPV) ? num_points : num_lines;
    out << "FIELD " << descriptor.name << " 1\n";
    out << descriptor.name << " " << descriptor.columns << " " << num_tuples << " " << token << "\n";
  }
  VTKUtils::append_file(out, field.tempfile);
  if (encoding == Encoding::Binary)
    out << "\n";
}

template <class ValueType> void VTKWriter<ValueType>::finalise() {
  points_buffer.commit();
  lines_buffer.commit();
  for (auto &field : sidecars)
    field.buffer->commit();

  File::OFStream out(path, std::ios::out | std::ios::binary | std::ios::trunc);

  out << VTKUtils::dataset_header(encoding, "POLYDATA");
  out << "POINTS " << num_points << " float\n";
  VTKUtils::append_file(out, points_tempfile);
  // Binary payloads must be separated from the following ASCII keyword by a
  //   newline; ASCII payloads already end each record with one.
  if (encoding == Encoding::Binary)
    out << "\n";

  out << "LINES " << num_lines << " " << lines_list_size << "\n";
  VTKUtils::append_file(out, lines_tempfile);
  if (encoding == Encoding::Binary)
    out << "\n";

  // Sidecar attributes (Stage 13): dpv fields under POINT_DATA, dps fields under
  //   CELL_DATA. The attribute-section count is the number of points / cells.
  const bool have_dpv = std::any_of(
      sidecars.begin(), sidecars.end(), [](const SidecarOutput &f) { return f.descriptor.role == FieldRole::DPV; });
  const bool have_dps = std::any_of(
      sidecars.begin(), sidecars.end(), [](const SidecarOutput &f) { return f.descriptor.role == FieldRole::DPS; });
  if (have_dpv) {
    out << "POINT_DATA " << num_points << "\n";
    for (const auto &field : sidecars)
      if (field.descriptor.role == FieldRole::DPV)
        append_sidecar(out, field);
  }
  if (have_dps) {
    out << "CELL_DATA " << num_lines << "\n";
    for (const auto &field : sidecars)
      if (field.descriptor.role == FieldRole::DPS)
        append_sidecar(out, field);
  }

  out.close();

  std::error_code ec;
  std::filesystem::remove(points_tempfile, ec);
  std::filesystem::remove(lines_tempfile, ec);
  for (const auto &field : sidecars)
    std::filesystem::remove(field.tempfile, ec);
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
                                                        Properties &properties,
                                                        FieldRegistry &,
                                                        const OptionalHeader &) const {
  return std::make_unique<VTKReader<float>>(path, properties);
}

std::unique_ptr<ReaderInterface<double>> VTK::read_double(const std::filesystem::path &path,
                                                          Properties &properties,
                                                          FieldRegistry &,
                                                          const OptionalHeader &) const {
  return std::make_unique<VTKReader<double>>(path, properties);
}

std::unique_ptr<WriterInterface<float>> VTK::create_float(const std::filesystem::path &path,
                                                          const Properties &properties,
                                                          const FieldRegistry &registry,
                                                          const OptionalHeader &) const {
  return std::make_unique<VTKWriter<float>>(path, properties, registry);
}

std::unique_ptr<WriterInterface<double>> VTK::create_double(const std::filesystem::path &path,
                                                            const Properties &properties,
                                                            const FieldRegistry &registry,
                                                            const OptionalHeader &) const {
  return std::make_unique<VTKWriter<double>>(path, properties, registry);
}

} // namespace Formats

} // namespace MR::DWI::Tractography
