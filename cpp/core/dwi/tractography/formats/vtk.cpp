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

#include "dwi/tractography/nonfinite.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
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

namespace {

//! \brief the byte size of one element of a sidecar block's native datatype.
size_t element_bytes(const DataType dtype) { return dtype.bytes(); }

} // namespace

template <class ValueType>
VTKReader<ValueType>::VTKReader(const std::filesystem::path &path, Properties &properties, FieldRegistry &registry)
    : registry(registry), current_streamline(0), current_vertex(0), current_index(0) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    throw Exception("unable to open VTK file \"" + path.string() + "\"");

  std::string line;

  // Parts 1-3: version line, optional description comments, and the ASCII/BINARY
  //   encoding keyword (shared with the ".vtx" reader; see VTKUtils).
  encoding = VTKUtils::parse_preamble(in, path, properties);

  // Part 4: dataset structure (POLYDATA POINTS + LINES), plus part 5 dataset
  //   attributes (POINT_DATA → dpv, CELL_DATA → dps; Stage 13).
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

  // Active dataset-attribute section: POINT_DATA tuples == num_points (dpv),
  //   CELL_DATA tuples == num_lines (dps). std::nullopt until the first such
  //   keyword is seen.
  std::optional<FieldRole> attribute_role;
  size_t attribute_tuples = 0;

  //! \brief skip or parse one sidecar block's payload of \a num_tuples × \a M
  //!   values of \a dtype, recording a SidecarBlock at the current position.
  auto record_block =
      [&](std::string_view name, FieldRole role, DataType dtype, size_t M, size_t num_tuples, bool ascii_color) {
        SidecarBlock block;
        block.name = std::string(name);
        block.role = role;
        block.dtype = dtype;
        block.columns = M;
        block.ordinal = 0;
        block.binary_offset = 0;
        block.cursor = 0;
        block.ascii_color = ascii_color && (encoding == Encoding::ASCII);
        const size_t num_values = num_tuples * M;
        if (encoding == Encoding::Binary) {
          block.binary_offset = static_cast<int64_t>(in.tellg());
          in.seekg(block.binary_offset + static_cast<int64_t>(num_values * element_bytes(dtype)), std::ios::beg);
        } else {
          block.ascii.resize(num_values);
          for (size_t i = 0; i != num_values; ++i) {
            double value = 0.0;
            if (!(in >> value))
              throw Exception("VTK file \"" + path.string() + "\" truncated within sidecar field \"" +
                              std::string(name) + "\"");
            block.ascii[i] = value;
          }
        }
        sidecars.push_back(std::move(block));
      };

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
      // Consume the LINES payload to reach any following attribute section. For
      //   binary, the block is validated below via the mmap (the stream is just
      //   advanced past it); for ASCII, validate sequential ordering and record
      //   the per-streamline vertex counts inline as the values are consumed.
      if (encoding == Encoding::Binary) {
        const size_t index_size = lines_int64 ? sizeof(int64_t) : sizeof(int32_t);
        in.seekg(lines_offset + static_cast<int64_t>(lines_list_size * index_size), std::ios::beg);
      } else {
        streamline_sizes.reserve(num_lines);
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
      continue;
    }

    // Part 5 dataset attributes: POINT_DATA / CELL_DATA open a section whose
    //   subsequent SCALARS / COLOR_SCALARS / FIELD attributes are sidecar fields.
    if (keyword == "POINT_DATA" || keyword == "CELL_DATA") {
      std::istringstream stream(line);
      std::string dummy;
      size_t declared = 0;
      stream >> dummy >> declared;
      attribute_role = (keyword == "POINT_DATA") ? FieldRole::DPV : FieldRole::DPS;
      attribute_tuples = (keyword == "POINT_DATA") ? num_points : num_lines;
      if (declared != attribute_tuples)
        throw Exception("VTK file \"" + path.string() + "\" " + keyword + " count (" + str(declared) +
                        ") does not match the number of " + (keyword == "POINT_DATA" ? "points" : "cells") + " (" +
                        str(attribute_tuples) + ")");
      continue;
    }

    if (keyword == "SCALARS" || keyword == "COLOR_SCALARS" || keyword == "FIELD") {
      if (!attribute_role.has_value())
        throw Exception("VTK file \"" + path.string() + "\" attribute \"" + keyword +
                        "\" appears outside any POINT_DATA / CELL_DATA section");
      std::istringstream stream(line);
      std::string dummy;
      std::string name;
      stream >> dummy >> name;
      if (keyword == "SCALARS") {
        // SCALARS name dataType [numComp]; a LOOKUP_TABLE line follows.
        std::string datatype;
        size_t num_comp = 1;
        stream >> datatype;
        if (!(stream >> num_comp))
          num_comp = 1;
        std::string lookup_line;
        if (!std::getline(in, lookup_line))
          throw Exception("VTK file \"" + path.string() + "\" truncated after SCALARS \"" + name + "\"");
        record_block(name,
                     *attribute_role,
                     VTKUtils::datatype_from_vtk_token(datatype, path),
                     num_comp,
                     attribute_tuples,
                     false);
      } else if (keyword == "COLOR_SCALARS") {
        // COLOR_SCALARS name nValues; uint8 components (native colour, e.g. RGB).
        //   In ASCII these are written as floats 0..1, but the sidecar dtype is
        //   recorded as the native uint8 the BINARY form carries (D7); the ASCII
        //   0..1 values are rescaled to 0..255 on read.
        size_t num_values = 0;
        stream >> num_values;
        record_block(name, *attribute_role, DataType(DataType::UInt8), num_values, attribute_tuples, true);
      } else {
        // FIELD name numArrays; each "arrayName M numTuples dataType" is a field.
        size_t num_arrays = 0;
        stream >> num_arrays;
        for (size_t a = 0; a != num_arrays; ++a) {
          std::string array_line;
          while (std::getline(in, array_line)) {
            std::string probe;
            std::istringstream probe_stream(array_line);
            probe_stream >> probe;
            if (!probe.empty())
              break;
          }
          std::istringstream array_stream(array_line);
          std::string array_name;
          size_t M = 0;
          size_t num_tuples = 0;
          std::string datatype;
          array_stream >> array_name >> M >> num_tuples >> datatype;
          if (num_tuples != attribute_tuples)
            throw Exception("VTK file \"" + path.string() + "\" FIELD array \"" + array_name + "\" tuple count (" +
                            str(num_tuples) + ") does not match the section size (" + str(attribute_tuples) + ")");
          record_block(
              array_name, *attribute_role, VTKUtils::datatype_from_vtk_token(datatype, path), M, num_tuples, false);
        }
      }
      continue;
    }

    // Attribute kinds carrying no per-streamline / per-vertex sidecar meaning are
    //   silently skipped; their payload is consumed so scanning can continue.
    if (keyword == "VECTORS" || keyword == "NORMALS" || keyword == "TENSORS" || keyword == "TEXTURE_COORDINATES" ||
        keyword == "LOOKUP_TABLE") {
      throw Exception("VTK file \"" + path.string() + "\" contains unsupported attribute \"" + keyword +
                      "\"; only SCALARS / COLOR_SCALARS / FIELD are carried as sidecar data");
    }

    // Any other dataset field (VERTICES / POLYGONS / TRIANGLE_STRIPS / etc.) is
    //   not part of a tractogram representation.
    throw Exception("VTK file \"" + path.string() + "\" contains unsupported dataset field \"" + keyword +
                    "\"; only POINTS and LINES are permitted");
  }

  if (!have_points)
    throw Exception("VTK file \"" + path.string() + "\" contains no POINTS data");

  // A file with no LINES contains no streamlines; this is a valid empty tractogram.
  if (!have_lines)
    num_lines = 0;

  // Validate the binary LINES block via the memory-map: verify sequential vertex
  //   ordering and record the per-streamline vertex count (the ASCII connectivity
  //   was already validated/recorded inline during the header scan above).
  if (have_lines && encoding == Encoding::Binary) {
    streamline_sizes.reserve(num_lines);
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

  // Register each discovered sidecar attribute on the field registry (§2.5),
  //   assigning its role-local ordinal; the per-item dps/dpv payloads are then
  //   addressed by that ordinal during streaming.
  for (auto &block : sidecars) {
    FieldDescriptor descriptor;
    descriptor.name = block.name;
    descriptor.role = block.role;
    descriptor.dtype = block.dtype;
    descriptor.columns = block.columns;
    descriptor.source = FieldSource::Internal;
    descriptor.ordinal = 0;
    block.ordinal = registry.add(std::move(descriptor));
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
  // Keep the sidecar block cursors in step even on the vertices-only read path,
  //   so a subsequent composite read is not misaligned (each dpv block advances
  //   by this streamline's vertex count, each dps block by one tuple).
  for (auto &block : sidecars)
    block.cursor += (block.role == FieldRole::DPV) ? count : 1;
  return true;
}

template <class ValueType> bool VTKReader<ValueType>::operator()(TractogramItem<ValueType> &item) {
  item.clear();
  if (current_streamline >= streamline_sizes.size())
    return false;
  const uint32_t count = streamline_sizes[current_streamline];
  item.streamline.set_index(current_index);
  // Vertices: advance current_vertex / current_streamline / current_index via the
  //   bare-streamline read so the two paths share the POINTS streaming logic.
  (*this)(item.streamline);

  // Sidecar payloads, addressed by their role-local ordinal (registry, §2.5).
  if (!sidecars.empty()) {
    item.dps.resize(registry.dps_count());
    item.dpv.resize(registry.dpv_count());
    for (auto &block : sidecars) {
      if (block.role == FieldRole::DPS)
        item.dps[block.ordinal] = read_dps(block);
      else
        item.dpv[block.ordinal] = read_dpv(block, count);
    }
  }
  return true;
}

namespace {

//! \brief common per-block streaming context passed to the dtype-generic readers.
template <class ValueType> struct SidecarReadContext {
  using Block = typename VTKReader<ValueType>::SidecarBlock;
  const Block &block;
  Encoding encoding;
  const std::byte *mmap_base; //!< null in ASCII mode
};

//! \brief dtype-generic reader of one M-component dps tuple (one streamline).
template <class ValueType> struct ReadDPSTuple {
  SidecarReadContext<ValueType> ctx;
  template <typename T> DPSValue operator()() const {
    const auto &block = ctx.block;
    ScalarOrVector<T> value(static_cast<Eigen::Index>(block.columns));
    // The vertices-only read advanced block.cursor past this tuple; step back one.
    const size_t tuple = block.cursor - 1;
    if (ctx.encoding == Encoding::Binary) {
      const int64_t offset = block.binary_offset + static_cast<int64_t>(tuple * block.columns * sizeof(T));
      VTKUtils::fetch_sidecar_tuple_BE<T>(ctx.mmap_base, offset, value.data(), block.columns);
    } else {
      const size_t base = tuple * block.columns;
      for (size_t c = 0; c != block.columns; ++c)
        value(0, static_cast<Eigen::Index>(c)) = static_cast<T>(block.ascii[base + c]);
    }
    return make_dps(std::move(value));
  }
};

//! \brief dtype-generic reader of an n_vertices × M dpv block (one streamline).
template <class ValueType> struct ReadDPVTuples {
  SidecarReadContext<ValueType> ctx;
  size_t n_vertices;
  template <typename T> DPVValue operator()() const {
    const auto &block = ctx.block;
    VectorOrMatrix<T> value(static_cast<Eigen::Index>(n_vertices), static_cast<Eigen::Index>(block.columns));
    // block.cursor was advanced by n_vertices on the vertex read; the first vertex
    //   of this streamline begins n_vertices tuples back.
    const size_t first_tuple = block.cursor - n_vertices;
    for (size_t r = 0; r != n_vertices; ++r) {
      const size_t tuple = first_tuple + r;
      if (ctx.encoding == Encoding::Binary) {
        const int64_t offset = block.binary_offset + static_cast<int64_t>(tuple * block.columns * sizeof(T));
        std::vector<T> row(block.columns);
        VTKUtils::fetch_sidecar_tuple_BE<T>(ctx.mmap_base, offset, row.data(), block.columns);
        for (size_t c = 0; c != block.columns; ++c)
          value(static_cast<Eigen::Index>(r), static_cast<Eigen::Index>(c)) = row[c];
      } else {
        const size_t base = tuple * block.columns;
        for (size_t c = 0; c != block.columns; ++c) {
          double raw = block.ascii[base + c];
          if (block.ascii_color)
            raw = std::round(raw * 255.0);
          value(static_cast<Eigen::Index>(r), static_cast<Eigen::Index>(c)) = static_cast<T>(raw);
        }
      }
    }
    return make_dpv(std::move(value));
  }
};

} // namespace

template <class ValueType> DPSValue VTKReader<ValueType>::read_dps(SidecarBlock &block) {
  const std::byte *const base = (mmap != nullptr) ? mmap->address() : nullptr;
  return dispatch_sidecar_datatype(block.dtype, ReadDPSTuple<ValueType>{{block, encoding, base}});
}

template <class ValueType> DPVValue VTKReader<ValueType>::read_dpv(SidecarBlock &block, size_t n_vertices) {
  const std::byte *const base = (mmap != nullptr) ? mmap->address() : nullptr;
  return dispatch_sidecar_datatype(block.dtype, ReadDPVTuples<ValueType>{{block, encoding, base}, n_vertices});
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
  enforce_vertices(tck, vtk_vertex_tolerance);
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

std::optional<VTKBinaryLayout> VTK::binary_layout(const std::filesystem::path &path) const {
  // Header-only parse: locate the POINTS and LINES blocks without reading either
  //   payload. Any condition that a raw-block consumer cannot serve (ASCII
  //   encoding, missing block, unsupported POINTS datatype) returns std::nullopt
  //   so the caller falls back to the streaming reader. The POINTS / LINES
  //   payload contents (notably connectivity contiguity) are NOT validated here.
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
  std::optional<int64_t> lines_offset;
  size_t num_lines = 0;
  size_t lines_list_size = 0;
  bool lines_int64 = false;

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
      if (type != "POLYDATA")
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

    if (keyword == "LINES") {
      std::istringstream stream(line);
      std::string dummy;
      stream >> dummy >> num_lines >> lines_list_size;
      lines_int64 = line.find("vtktypeint64") != std::string::npos;
      lines_offset = static_cast<int64_t>(in.tellg());
      const size_t index_size = lines_int64 ? sizeof(int64_t) : sizeof(int32_t);
      // Advance past the LINES payload; the consumer parses it from the mmap.
      in.seekg(*lines_offset + static_cast<int64_t>(lines_list_size * index_size), std::ios::beg);
      continue;
    }

    // POINTS and LINES are the only structures a raw-block consumer serves; once
    //   both are located, any trailing dataset attributes (POINT_DATA /
    //   CELL_DATA sidecars) are irrelevant to the vertex / topology fast path.
    //   Stop scanning as soon as both are in hand.
    if (points_offset.has_value() && lines_offset.has_value())
      break;
  }

  if (!points_offset.has_value())
    return std::nullopt;
  // A file with no LINES is a valid empty tractogram, but offers nothing for the
  //   fast path to build; defer it to the streaming reader.
  if (!lines_offset.has_value())
    return std::nullopt;

  // Legacy VTK binary is big-endian by spec; report the explicit byte order so
  //   the consumer can choose a verbatim copy (big-endian host) or a staging
  //   byte-swap (little-endian host).
  const DataType points_datatype = (point_type == PointDataType::Float32)
                                       ? DataType(DataType::Float32 | DataType::BigEndian)
                                       : DataType(DataType::Float64 | DataType::BigEndian);

  return VTKBinaryLayout{
      *points_offset, num_points, points_datatype, *lines_offset, num_lines, lines_list_size, lines_int64};
}

std::unique_ptr<ReaderInterface<float>> VTK::read_float(const std::filesystem::path &path,
                                                        Properties &properties,
                                                        FieldRegistry &registry,
                                                        const OptionalHeader &) const {
  return std::make_unique<VTKReader<float>>(path, properties, registry);
}

std::unique_ptr<ReaderInterface<double>> VTK::read_double(const std::filesystem::path &path,
                                                          Properties &properties,
                                                          FieldRegistry &registry,
                                                          const OptionalHeader &) const {
  return std::make_unique<VTKReader<double>>(path, properties, registry);
}

std::unique_ptr<WriterInterface<float>> VTK::create_float(const std::filesystem::path &path,
                                                          const Properties &properties,
                                                          const FieldRegistry &registry,
                                                          const OptionalHeader &,
                                                          const WriteOptions &options) const {
  return std::make_unique<VTKWriter<float>>(path, properties, registry);
}

std::unique_ptr<WriterInterface<double>> VTK::create_double(const std::filesystem::path &path,
                                                            const Properties &properties,
                                                            const FieldRegistry &registry,
                                                            const OptionalHeader &,
                                                            const WriteOptions &options) const {
  return std::make_unique<VTKWriter<double>>(path, properties, registry);
}

} // namespace Formats

} // namespace MR::DWI::Tractography
