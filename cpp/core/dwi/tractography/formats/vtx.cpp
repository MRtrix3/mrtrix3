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

#include "dwi/tractography/formats/vtx.h"

#include "dwi/tractography/nonfinite.h"

#include <array>
#include <fstream>
#include <optional>
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

#include "dwi/tractography/formats/vtk_utils.h"

namespace MR::DWI::Tractography {

namespace VTKUtils = Formats::VTKUtils;
using VTKUtils::Encoding;
using VTKUtils::PointDataType;

/* ************************************************************************ */
/*                          VTXReader<ValueType>                           */
/* ************************************************************************ */

template <class ValueType>
VTXReader<ValueType>::VTXReader(const std::filesystem::path &path, Properties &properties)
    : num_streamlines(0),
      offsets_offset(0),
      current_streamline(0),
      current_vertex(0),
      previous_offset_end(-1),
      current_index(0) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    throw Exception("unable to open VTX file \"" + path.string() + "\"");

  std::string line;

  // Parts 1-3: version line, optional description comments, and the ASCII/BINARY
  //   encoding keyword (shared with the ".vtk" reader; see VTKUtils).
  encoding = VTKUtils::parse_preamble(in, path, properties);

  // Part 4: dataset structure; only STREAMLINES with POINTS and OFFSETS admitted.
  bool have_points = false;
  bool have_offsets = false;
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
      if (type != "STREAMLINES")
        throw Exception("VTX file \"" + path.string() + "\" is not STREAMLINES (DATASET " + type +
                        "); only POINTS/OFFSETS STREAMLINES is supported");
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
        throw Exception("VTX file \"" + path.string() + "\" POINTS datatype \"" + datatype +
                        "\" is unsupported (expected float or double)");
      have_points = true;
      // Expose the on-disk vertex datatype for downstream consumers (e.g.
      //   mrview's GPU vertex-width selection); header-only, no vertex read.
      properties.vertex_datatype =
          DataType((point_type == PointDataType::Float32) ? DataType::Float32 : DataType::Float64);
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
            throw Exception("VTX file \"" + path.string() + "\" truncated while reading ASCII POINTS data");
          ascii_points[i] = static_cast<ValueType>(value);
        }
      }
      continue;
    }

    if (keyword == "OFFSETS") {
      std::istringstream stream(line);
      std::string dummy;
      std::string datatype;
      stream >> dummy >> num_streamlines >> datatype;
      // A "vtktypeint64" qualifier selects 64-bit indices; "int"/"vtktypeint32"
      //   select 32-bit. The writer emits "vtktypeint64".
      if (datatype == "vtktypeint64" || datatype == "long" || datatype == "vtkIdType")
        offset_type = OffsetType::Int64;
      else if (datatype == "int" || datatype == "vtktypeint32")
        offset_type = OffsetType::Int32;
      else
        throw Exception("VTX file \"" + path.string() + "\" OFFSETS datatype \"" + datatype +
                        "\" is unsupported (expected int or vtktypeint64)");
      have_offsets = true;

      if (encoding == Encoding::Binary) {
        offsets_offset = static_cast<int64_t>(in.tellg());
      } else {
        // ASCII OFFSETS follow on the same input stream; parse them into RAM so
        //   that streaming can later read one offset at a time without a stream
        //   re-position. Per the streaming requirement, the per-streamline vertex
        //   count is still computed during operator(), as the difference between
        //   consecutive offsets, never up-front.
        ascii_offsets.resize(num_streamlines);
        for (size_t i = 0; i != num_streamlines; ++i) {
          int64_t value = 0;
          if (!(in >> value))
            throw Exception("VTX file \"" + path.string() + "\" truncated while reading ASCII OFFSETS data");
          ascii_offsets[i] = value;
        }
      }
      // OFFSETS is the last field of interest; stop scanning the header.
      break;
    }

    // Any other dataset field is not part of a STREAMLINES tractogram.
    throw Exception("VTX file \"" + path.string() + "\" contains unsupported dataset field \"" + keyword +
                    "\"; only POINTS and OFFSETS are permitted");
  }

  if (!have_points)
    throw Exception("VTX file \"" + path.string() + "\" contains no POINTS data");

  // A file with no OFFSETS contains no streamlines; this is a valid empty tractogram.
  if (!have_offsets)
    num_streamlines = 0;

  // Establish the memory-map for binary POINTS / OFFSETS access. OFFSETS are read
  //   incrementally through this map during streaming (operator()), one at a time.
  //   Construct the shared POINTS accessor (VTKUtils) over the same map (binary)
  //   or the RAM-resident ASCII coordinates.
  if (encoding == Encoding::Binary) {
    mmap = std::make_shared<File::MMap>(File::Entry(path), false, true);
    points = std::make_unique<VTKUtils::PointReader<ValueType>>(mmap, points_offset, point_type, num_points);
  } else {
    points = std::make_unique<VTKUtils::PointReader<ValueType>>(std::move(ascii_points), num_points);
  }
}

template <class ValueType> VTXReader<ValueType>::~VTXReader() = default;

template <class ValueType> int64_t VTXReader<ValueType>::get_offset_end(size_t j) const {
  if (encoding == Encoding::ASCII)
    return ascii_offsets[j];
  // Read a single OFFSETS entry directly from the memory-map; nothing of the
  //   OFFSETS block is parsed ahead of the streamline currently being yielded.
  const std::byte *const base = mmap->address();
  const size_t index_size = (offset_type == OffsetType::Int64) ? sizeof(int64_t) : sizeof(int32_t);
  const int64_t pos = offsets_offset + static_cast<int64_t>(j * index_size);
  if (pos + static_cast<int64_t>(index_size) > mmap->size())
    throw Exception("VTX file truncated within OFFSETS block");
  return (offset_type == OffsetType::Int64) ? Raw::fetch_BE<int64_t>(base + pos)
                                            : static_cast<int64_t>(Raw::fetch_BE<int32_t>(base + pos));
}

template <class ValueType> bool VTXReader<ValueType>::operator()(Streamline<ValueType> &tck) {
  tck.clear();
  if (current_streamline >= num_streamlines)
    return false;

  // Streaming vertex-count determination: read this streamline's END offset and
  //   subtract the previous streamline's END offset (offsetEnd[-1] = -1). The
  //   streamline spans points offsetEnd[j-1]+1 .. offsetEnd[j].
  const int64_t offset_end = get_offset_end(current_streamline);
  const int64_t count = offset_end - previous_offset_end;
  if (count < 0)
    throw Exception("VTX file OFFSETS are not monotonically increasing");
  if (current_vertex + static_cast<size_t>(count) > points->size())
    throw Exception("VTX file OFFSETS reference a vertex beyond the POINTS block");

  tck.set_index(current_index++);
  for (int64_t v = 0; v != count; ++v)
    tck.push_back(points->get_point(current_vertex++));

  previous_offset_end = offset_end;
  ++current_streamline;
  return true;
}

/* ************************************************************************ */
/*                          VTXWriter<ValueType>                           */
/* ************************************************************************ */

template <class ValueType>
VTXWriter<ValueType>::VTXWriter(const std::filesystem::path &path, const Properties &properties)
    : path(path),
      points_buffer(File::Config::get_int("TrackWriterBufferSize", 16777216), 1),
      offsets_buffer(File::Config::get_int("TrackWriterBufferSize", 16777216), 1),
      num_points(0),
      num_streamlines(0) {
  if (path.extension() != ".vtx")
    throw Exception("output VTX track files must use the .vtx suffix");

  // The "-ascii" option (declared by commands that write VTK-derived formats,
  //   e.g. tckconvert) selects the ASCII encoding; the BINARY (big-endian)
  //   encoding is the default.
  encoding = App::get_options("ascii").empty() ? Encoding::Binary : Encoding::ASCII;

  (void)properties;

  // Verify the output can be created, then truncate any pre-existing content.
  App::check_overwrite(path);
  {
    File::OFStream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out.good())
      throw Exception("unable to create VTX output file \"" + path.string() + "\"");
  }

  points_tempfile = File::create_tempfile(0, ".vtxpoints");
  offsets_tempfile = File::create_tempfile(0, ".vtxoffsets");

  const std::filesystem::path points_path = points_tempfile;
  const std::filesystem::path offsets_path = offsets_tempfile;
  points_buffer.set_flush_callback(
      [points_path](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &) {
        VTKUtils::append_bytes(points_path, data, size);
      });
  offsets_buffer.set_flush_callback(
      [offsets_path](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &) {
        VTKUtils::append_bytes(offsets_path, data, size);
      });
}

template <class ValueType> void VTXWriter<ValueType>::add_offset(int64_t offset_end) {
  if (encoding == Encoding::ASCII) {
    std::ostringstream stream;
    stream << offset_end << "\n";
    const std::string text = stream.str();
    offsets_buffer.add(reinterpret_cast<const std::byte *>(text.data()), text.size());
  } else {
    std::array<int64_t, 1> raw{};
    Raw::store_BE<int64_t>(offset_end, raw.data(), 0);
    offsets_buffer.add(reinterpret_cast<const std::byte *>(raw.data()), sizeof(raw));
  }
}

template <class ValueType> bool VTXWriter<ValueType>::operator()(const Streamline<ValueType> &tck) {
  enforce_vertices(tck, vtx_vertex_tolerance);
  for (const auto &pos : tck)
    VTKUtils::write_point<ValueType>(points_buffer, encoding, pos);
  num_points += tck.size();
  ++num_streamlines;
  // OFFSETS store the END vertex index of streamline j (offsetEnd[j]); with
  //   running vertex total num_points, the last index of this streamline is
  //   num_points - 1.
  add_offset(static_cast<int64_t>(num_points) - 1);
  return true;
}

template <class ValueType> void VTXWriter<ValueType>::finalise() {
  points_buffer.commit();
  offsets_buffer.commit();

  File::OFStream out(path, std::ios::out | std::ios::binary | std::ios::trunc);

  out << VTKUtils::dataset_header(encoding, "STREAMLINES");
  out << "POINTS " << num_points << " float\n";
  VTKUtils::append_file(out, points_tempfile);
  // Binary payloads must be separated from the following ASCII keyword by a
  //   newline; ASCII payloads already end each record with one.
  if (encoding == Encoding::Binary)
    out << "\n";

  // The number of OFFSETS entries equals the number of streamlines.
  out << "OFFSETS " << num_streamlines << " vtktypeint64\n";
  VTKUtils::append_file(out, offsets_tempfile);
  if (encoding == Encoding::Binary)
    out << "\n";

  out.close();

  std::error_code ec;
  std::filesystem::remove(points_tempfile, ec);
  std::filesystem::remove(offsets_tempfile, ec);
}

template <class ValueType> VTXWriter<ValueType>::~VTXWriter() {
  try {
    finalise();
  } catch (Exception &e) {
    Exception(e, "VTX tractography file not properly finalised").display();
  }
}

/* ************************************************************************ */
/*               Explicit instantiation for float and double              */
/* ************************************************************************ */

template class VTXReader<float>;
template class VTXReader<double>;
template class VTXWriter<float>;
template class VTXWriter<double>;

namespace Formats {

bool VTX::handles(const std::filesystem::path &path) const { return path.extension() == ".vtx"; }

std::optional<VTXBinaryLayout> VTX::binary_layout(const std::filesystem::path &path) const {
  // Header-only parse: locate the POINTS and OFFSETS blocks without reading
  //   either payload. Any condition that a raw-block consumer cannot serve
  //   (ASCII encoding, missing block, unsupported POINTS datatype) returns
  //   std::nullopt so the caller falls back to the streaming reader. The POINTS /
  //   OFFSETS payload contents are NOT validated here.
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return std::nullopt;

  Properties scratch_properties;
  Encoding encoding;
  try {
    encoding = VTKUtils::parse_preamble(in, path, scratch_properties);
  } catch (Exception &) {
    return std::nullopt;
  }
  // A raw-block consumer maps a contiguous on-disk array; ASCII has none.
  if (encoding != Encoding::Binary)
    return std::nullopt;

  std::optional<int64_t> points_offset;
  size_t num_points = 0;
  PointDataType point_type = PointDataType::Float32;
  std::optional<int64_t> offsets_offset;
  size_t num_streamlines = 0;
  bool offsets_int64 = false;

  std::string line;
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
      if (type != "STREAMLINES")
        return std::nullopt;
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
        return std::nullopt;
      const size_t element_size = (point_type == PointDataType::Float32) ? sizeof(float) : sizeof(double);
      points_offset = static_cast<int64_t>(in.tellg());
      // Advance past the POINTS payload to reach the next keyword.
      in.seekg(*points_offset + static_cast<int64_t>(3 * num_points * element_size), std::ios::beg);
      continue;
    }

    if (keyword == "OFFSETS") {
      std::istringstream stream(line);
      std::string dummy;
      std::string datatype;
      stream >> dummy >> num_streamlines >> datatype;
      if (datatype == "vtktypeint64" || datatype == "long" || datatype == "vtkIdType")
        offsets_int64 = true;
      else if (datatype == "int" || datatype == "vtktypeint32")
        offsets_int64 = false;
      else
        return std::nullopt;
      offsets_offset = static_cast<int64_t>(in.tellg());
      // OFFSETS is the last field of interest; stop scanning the header.
      break;
    }

    // POINTS and OFFSETS are the only structures a raw-block consumer serves;
    //   any other dataset field is not part of a STREAMLINES tractogram.
    return std::nullopt;
  }

  if (!points_offset.has_value())
    return std::nullopt;
  // A file with no OFFSETS is a valid empty tractogram, but offers nothing for
  //   the fast path to build; defer it to the streaming reader.
  if (!offsets_offset.has_value())
    return std::nullopt;

  // ".vtx" binary is big-endian by spec; report the explicit byte order so the
  //   consumer can choose a verbatim copy (big-endian host) or a staging
  //   byte-swap (little-endian host).
  const DataType points_datatype = (point_type == PointDataType::Float32)
                                       ? DataType(DataType::Float32 | DataType::BigEndian)
                                       : DataType(DataType::Float64 | DataType::BigEndian);

  return VTXBinaryLayout{*points_offset, num_points, points_datatype, *offsets_offset, num_streamlines, offsets_int64};
}

std::unique_ptr<ReaderInterface<float>> VTX::read_float(const std::filesystem::path &path,
                                                        Properties &properties,
                                                        FieldRegistry &,
                                                        const OptionalHeader &) const {
  return std::make_unique<VTXReader<float>>(path, properties);
}

std::unique_ptr<ReaderInterface<double>> VTX::read_double(const std::filesystem::path &path,
                                                          Properties &properties,
                                                          FieldRegistry &,
                                                          const OptionalHeader &) const {
  return std::make_unique<VTXReader<double>>(path, properties);
}

std::unique_ptr<WriterInterface<float>> VTX::create_float(const std::filesystem::path &path,
                                                          const Properties &properties,
                                                          const FieldRegistry &,
                                                          const OptionalHeader &,
                                                          const WriteOptions &options) const {
  return std::make_unique<VTXWriter<float>>(path, properties);
}

std::unique_ptr<WriterInterface<double>> VTX::create_double(const std::filesystem::path &path,
                                                            const Properties &properties,
                                                            const FieldRegistry &,
                                                            const OptionalHeader &,
                                                            const WriteOptions &options) const {
  return std::make_unique<VTXWriter<double>>(path, properties);
}

} // namespace Formats

} // namespace MR::DWI::Tractography
