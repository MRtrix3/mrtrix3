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

#include "dwi/tractography/formats/trk.h"

#include "dwi/tractography/nonfinite.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "app.h"
#include "exception.h"
#include "file/config.h"
#include "file/entry.h"
#include "file/mmap.h"
#include "file/ofstream.h"
#include "file/temp.h"
#include "header.h"
#include "match_variant.h"
#include "raw.h"

#include "dwi/tractography/sidecar_value.h"

namespace MR::DWI::Tractography {

namespace TRKUtils = Formats::TRKUtils;

namespace {

//! the size in bytes of one on-disk ".trk" coordinate / scalar / property value
constexpr size_t trk_value_bytes = sizeof(float);

//! \brief read a fixed-width header name array entry into a trimmed std::string.
/*! TrackVis names are NUL-padded within a 20-char field; copy up to the first
 * NUL (or the field width) so the in-memory name has no trailing padding. */
std::string read_name(const std::array<char, TRKUtils::name_length> &field) {
  size_t length = 0;
  while (length != TRKUtils::name_length && field[length] != '\0')
    ++length;
  return std::string(field.data(), length);
}

//! \brief write a std::string into a fixed-width NUL-padded header name field.
void write_name(std::array<char, TRKUtils::name_length> &field, std::string_view name) {
  field.fill('\0');
  const size_t length = std::min(name.size(), TRKUtils::name_length);
  std::memcpy(field.data(), name.data(), length);
}

//! \brief in-place byte-swap of every multi-byte field of a ".trk" header.
/*! Invoked when the \c hdr_size sentinel reveals the file is opposite-endian to
 * this host; single-byte (char / uint8) fields are unaffected. */
void byte_swap_header(TRKUtils::Header &header) {
  for (size_t axis = 0; axis != 3; ++axis) {
    header.dim[axis] = ByteOrder::swap(header.dim[axis]);
    header.voxel_size[axis] = ByteOrder::swap(header.voxel_size[axis]);
    header.origin[axis] = ByteOrder::swap(header.origin[axis]);
  }
  header.n_scalars = ByteOrder::swap(header.n_scalars);
  header.n_properties = ByteOrder::swap(header.n_properties);
  for (size_t row = 0; row != 4; ++row)
    for (size_t col = 0; col != 4; ++col)
      header.vox_to_ras[row][col] = ByteOrder::swap(header.vox_to_ras[row][col]);
  for (size_t i = 0; i != 6; ++i)
    header.image_orientation_patient[i] = ByteOrder::swap(header.image_orientation_patient[i]);
  header.n_count = ByteOrder::swap(header.n_count);
  header.version = ByteOrder::swap(header.version);
  header.hdr_size = ByteOrder::swap(header.hdr_size);
}

//! \brief build the voxel→scanner transform a ".trk" header describes.
/*! The header's \c vox_to_ras maps a voxel index (crs) to scanner-space RAS xyz;
 * an unrecorded affine (vox_to_ras[3][3]==0) falls back to identity so the grid
 * is axis-aligned with a corner origin. This is the transform applied after the
 * mm→voxel-index division by the spacing. */
transform_type vox_to_ras_transform(const TRKUtils::Header &header) {
  transform_type T;
  T.setIdentity();
  if (std::fabs(header.vox_to_ras[3][3]) > 0.0F) {
    for (size_t row = 0; row != 3; ++row)
      for (size_t col = 0; col != 4; ++col)
        T(row, col) = static_cast<double>(header.vox_to_ras[row][col]);
  }
  return T;
}

} // namespace

/* ************************************************************************ */
/*                          TRKReader<ValueType>                           */
/* ************************************************************************ */

template <class ValueType>
TRKReader<ValueType>::TRKReader(const std::filesystem::path &path,
                                Properties &properties,
                                FieldRegistry &registry,
                                const OptionalHeader &grid)
    : registry(registry),
      position(static_cast<int64_t>(TRKUtils::header_bytes)),
      end(0),
      byte_swapped(false),
      n_scalars(0),
      n_properties(0),
      voxel_size{1.0, 1.0, 1.0},
      current_index(0) {
  mmap = std::make_shared<File::MMap>(File::Entry(path), false, false);
  end = mmap->size();
  if (end < static_cast<int64_t>(TRKUtils::header_bytes))
    throw Exception("TrackVis \".trk\" file \"" + path.string() + "\" is smaller than its 1000-byte header");

  TRKUtils::Header header;
  std::memcpy(&header, mmap->address(), TRKUtils::header_bytes);

  // The hdr_size field must read as 1000; if not, the file is opposite-endian to
  //   this host and every multi-byte field is byte-swapped on ingest.
  if (header.hdr_size != TRKUtils::header_size_sentinel) {
    byte_swap_header(header);
    if (header.hdr_size != TRKUtils::header_size_sentinel)
      throw Exception("TrackVis \".trk\" file \"" + path.string() + "\" has an invalid header size field (" +
                      str(header.hdr_size) + "); not a recognised \".trk\" file");
    byte_swapped = true;
  }

  if (std::string_view(header.id_string.data(), 5) != "TRACK")
    throw Exception("file \"" + path.string() + "\" is not a TrackVis \".trk\" file (bad id string)");

  for (size_t axis = 0; axis != 3; ++axis)
    voxel_size[axis] = (std::fabs(header.voxel_size[axis]) > 0.0F) ? static_cast<double>(header.voxel_size[axis]) : 1.0;

  n_scalars = static_cast<size_t>(std::max<int16_t>(0, header.n_scalars));
  n_properties = static_cast<size_t>(std::max<int16_t>(0, header.n_properties));
  if (n_scalars > TRKUtils::max_named_fields || n_properties > TRKUtils::max_named_fields)
    throw Exception("TrackVis \".trk\" file \"" + path.string() + "\" declares more than " +
                    str(TRKUtils::max_named_fields) + " scalars/properties");

  // The grid orientation/origin: prefer the supplied reference Header's
  //   voxel→scanner transform; otherwise fall back to the file's own vox_to_ras
  //   so a ".trk" → ".trk" round-trip is exact.
  if (grid.has_value()) {
    voxel2scanner = Transform(grid->get()).voxel2scanner;
    for (size_t axis = 0; axis != 3; ++axis)
      voxel_size[axis] = grid->get().spacing(axis);
  } else {
    voxel2scanner = vox_to_ras_transform(header);
  }

  // Preserve grid metadata into the Properties for a faithful round-trip.
  properties["trk_dim"] = str(header.dim[0]) + "," + str(header.dim[1]) + "," + str(header.dim[2]);
  properties["trk_voxel_size"] = str(voxel_size[0]) + "," + str(voxel_size[1]) + "," + str(voxel_size[2]);
  {
    std::string voxel_order(header.voxel_order.begin(), header.voxel_order.end());
    voxel_order.erase(std::find(voxel_order.begin(), voxel_order.end(), '\0'), voxel_order.end());
    if (!voxel_order.empty())
      properties["trk_voxel_order"] = voxel_order;
  }

  // Register each scalar as a dpv field and each property as a dps field; per the
  //   format spec these are float32 on disk (D7: carried as native float). Unnamed
  //   columns are given a positional default name.
  for (size_t i = 0; i != n_scalars; ++i) {
    FieldDescriptor descriptor;
    descriptor.name = read_name(header.scalar_name[i]);
    if (descriptor.name.empty())
      descriptor.name = "scalar_" + str(i);
    descriptor.role = FieldRole::DPV;
    descriptor.dtype = DataType::native(DataType(DataType::Float32));
    descriptor.columns = 1;
    descriptor.source = FieldSource::Internal;
    descriptor.ordinal = 0;
    scalar_ordinals.push_back(registry.add(std::move(descriptor)));
  }
  for (size_t i = 0; i != n_properties; ++i) {
    FieldDescriptor descriptor;
    descriptor.name = read_name(header.property_name[i]);
    if (descriptor.name.empty())
      descriptor.name = "property_" + str(i);
    descriptor.role = FieldRole::DPS;
    descriptor.dtype = DataType::native(DataType(DataType::Float32));
    descriptor.columns = 1;
    descriptor.source = FieldSource::Internal;
    descriptor.ordinal = 0;
    property_ordinals.push_back(registry.add(std::move(descriptor)));
  }
}

template <class ValueType> TRKReader<ValueType>::~TRKReader() = default;

template <class ValueType>
bool TRKReader<ValueType>::read_record(Streamline<ValueType> &tck, TractogramItem<ValueType> *item) {
  tck.clear();
  if (position + static_cast<int64_t>(sizeof(int32_t)) > end)
    return false;

  const std::byte *const base = mmap->address();
  const int32_t npoints =
      byte_swapped ? Raw::fetch_BE<int32_t>(base + position) : Raw::fetch_LE<int32_t>(base + position);
  if (npoints < 0)
    throw Exception("malformed TrackVis \".trk\" record: negative vertex count (" + str(npoints) + ")");
  position += static_cast<int64_t>(sizeof(int32_t));

  const size_t per_vertex_floats = 3 + n_scalars;
  const int64_t vertices_bytes =
      static_cast<int64_t>(npoints) * static_cast<int64_t>(per_vertex_floats * trk_value_bytes);
  const int64_t properties_bytes = static_cast<int64_t>(n_properties * trk_value_bytes);
  if (position + vertices_bytes + properties_bytes > end)
    throw Exception("malformed TrackVis \".trk\" file: streamline record overruns the file");

  // Prepare the dpv columns (one VectorOrMatrix<float> of n_vertices rows per
  //   scalar field) and dps slots when capturing sidecars.
  std::vector<VectorOrMatrix<float>> scalars;
  if (item != nullptr && n_scalars != 0) {
    scalars.resize(n_scalars);
    for (auto &column : scalars)
      column.resize(static_cast<Eigen::Index>(npoints), 1);
  }

  tck.set_index(current_index);
  if (item != nullptr)
    item->streamline.set_index(current_index);
  ++current_index;

  int64_t cursor = position;
  for (int32_t v = 0; v != npoints; ++v) {
    Eigen::Vector3d point_mm;
    for (size_t axis = 0; axis != 3; ++axis) {
      const float value = byte_swapped ? Raw::fetch_BE<float>(base + cursor) : Raw::fetch_LE<float>(base + cursor);
      point_mm[axis] = static_cast<double>(value);
      cursor += static_cast<int64_t>(trk_value_bytes);
    }
    // mm (voxel space, corner-referenced) → fractional voxel index → scanner RAS.
    const Eigen::Vector3d voxel(point_mm[0] / voxel_size[0], point_mm[1] / voxel_size[1], point_mm[2] / voxel_size[2]);
    const Eigen::Vector3d scanner = voxel2scanner * voxel;
    tck.push_back(scanner.cast<ValueType>());

    for (size_t s = 0; s != n_scalars; ++s) {
      const float value = byte_swapped ? Raw::fetch_BE<float>(base + cursor) : Raw::fetch_LE<float>(base + cursor);
      cursor += static_cast<int64_t>(trk_value_bytes);
      if (item != nullptr)
        scalars[s](static_cast<Eigen::Index>(v), 0) = value;
    }
  }

  // Per-streamline properties (dps), one float each.
  std::vector<float> property_values(n_properties);
  for (size_t p = 0; p != n_properties; ++p) {
    const float value = byte_swapped ? Raw::fetch_BE<float>(base + cursor) : Raw::fetch_LE<float>(base + cursor);
    cursor += static_cast<int64_t>(trk_value_bytes);
    property_values[p] = value;
  }

  position = cursor;

  if (item != nullptr) {
    item->dps.resize(registry.dps_count());
    item->dpv.resize(registry.dpv_count());
    for (size_t s = 0; s != n_scalars; ++s)
      item->dpv[scalar_ordinals[s]] = make_dpv(std::move(scalars[s]));
    for (size_t p = 0; p != n_properties; ++p) {
      ScalarOrVector<float> value(1);
      value(0, 0) = property_values[p];
      item->dps[property_ordinals[p]] = make_dps(std::move(value));
    }
  }
  return true;
}

template <class ValueType> bool TRKReader<ValueType>::operator()(Streamline<ValueType> &tck) {
  return read_record(tck, nullptr);
}

template <class ValueType> bool TRKReader<ValueType>::operator()(TractogramItem<ValueType> &item) {
  item.clear();
  return read_record(item.streamline, &item);
}

/* ************************************************************************ */
/*                          TRKWriter<ValueType>                           */
/* ************************************************************************ */

template <class ValueType>
TRKWriter<ValueType>::TRKWriter(const std::filesystem::path &path,
                                const Properties &properties,
                                const FieldRegistry &registry,
                                const OptionalHeader &grid)
    : path(path),
      registry(registry),
      voxel_size{1.0F, 1.0F, 1.0F},
      dim{1, 1, 1},
      n_scalars(0),
      n_properties(0),
      body_buffer(File::Config::get_int("TrackWriterBufferSize", 16777216), 1),
      num_streamlines(0) {
  if (path.extension() != ".trk")
    throw Exception("output TrackVis track files must use the .trk suffix");

  App::check_overwrite(path);

  // The voxel→RAS affine defaults to identity (axis-aligned, corner origin), so a
  //   ".trk" written without a reference grid round-trips exactly with the reader.
  for (size_t row = 0; row != 4; ++row)
    for (size_t col = 0; col != 4; ++col)
      vox_to_ras[row][col] = (row == col) ? 1.0F : 0.0F;
  scanner2voxel.setIdentity();

  if (grid.has_value()) {
    // A reference grid fixes the full geometry: the header carries that grid's
    //   spacing, dimensions and voxel→RAS affine, and scanner→voxel is its inverse.
    const Header &header = grid->get();
    scanner2voxel = Transform(header).scanner2voxel;
    const transform_type v2s = Transform(header).voxel2scanner;
    for (size_t axis = 0; axis != 3; ++axis) {
      voxel_size[axis] = static_cast<float>(header.spacing(axis));
      dim[axis] = static_cast<int16_t>(header.size(axis));
    }
    for (size_t row = 0; row != 3; ++row)
      for (size_t col = 0; col != 4; ++col)
        vox_to_ras[row][col] = static_cast<float>(v2s(row, col));
  } else {
    // Recover spacing / dimensions from any "trk_*" properties left by a prior
    //   ".trk" read so a ".trk" → ".trk" conversion preserves the grid metadata.
    auto vs = properties.find("trk_voxel_size");
    if (vs != properties.end()) {
      const auto values = MR::parse_floats(vs->second);
      for (size_t axis = 0; axis != 3 && axis != values.size(); ++axis)
        voxel_size[axis] = static_cast<float>(values[axis]);
    }
    auto dm = properties.find("trk_dim");
    if (dm != properties.end()) {
      const auto values = MR::parse_ints<int64_t>(dm->second);
      for (size_t axis = 0; axis != 3 && axis != values.size(); ++axis)
        dim[axis] = static_cast<int16_t>(values[axis]);
    }
  }

  // Map the registered dpv fields to per-vertex scalars and the dps fields to
  //   per-streamline properties; each column of an M>1 field is one ".trk"
  //   scalar/property. The reserved "weight" field rides on streamline.weight and
  //   is not a generic ".trk" property, so it is not emitted here.
  for (const auto &field : registry) {
    if (field.role == FieldRole::DPV) {
      SidecarOutput output;
      output.descriptor = field;
      output.ordinal = field.ordinal;
      n_scalars += field.columns;
      scalar_fields.push_back(std::move(output));
    } else if (field.role == FieldRole::DPS) {
      SidecarOutput output;
      output.descriptor = field;
      output.ordinal = field.ordinal;
      n_properties += field.columns;
      property_fields.push_back(std::move(output));
    }
  }
  if (n_scalars > TRKUtils::max_named_fields)
    throw Exception("TrackVis \".trk\" supports at most " + str(TRKUtils::max_named_fields) +
                    " per-vertex scalar columns; output requires " + str(n_scalars));
  if (n_properties > TRKUtils::max_named_fields)
    throw Exception("TrackVis \".trk\" supports at most " + str(TRKUtils::max_named_fields) +
                    " per-streamline property columns; output requires " + str(n_properties));

  // Stream the body to a temporary file; the header is prepended on finalisation.
  body_tempfile = File::create_tempfile(0, ".trkbody");
  const std::filesystem::path body_path = body_tempfile;
  body_buffer.set_flush_callback([body_path](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &) {
    File::OFStream out(body_path, std::ios::out | std::ios::binary | std::ios::app);
    out.write(reinterpret_cast<const char *>(data), size);
  });
}

template <class ValueType> bool TRKWriter<ValueType>::operator()(const Streamline<ValueType> &tck) {
  enforce_vertices(tck, trk_vertex_tolerance);
  // Vertices-only write: no scalars/properties (this path is taken only when the
  //   output registry has no dpv/dps fields).
  const size_t npoints = tck.size();
  std::vector<std::byte> record(sizeof(int32_t) + npoints * 3 * trk_value_bytes);
  Raw::store_LE<int32_t>(static_cast<int32_t>(npoints), record.data(), 0);
  size_t offset = sizeof(int32_t);
  for (const auto &pos : tck) {
    const Eigen::Vector3d voxel = scanner2voxel * pos.template cast<double>();
    for (size_t axis = 0; axis != 3; ++axis) {
      const float mm = static_cast<float>(voxel[axis] * voxel_size[axis]);
      Raw::store_LE<float>(mm, record.data() + offset);
      offset += trk_value_bytes;
    }
  }
  body_buffer.add(record.data(), record.size());
  ++num_streamlines;
  return true;
}

template <class ValueType> bool TRKWriter<ValueType>::operator()(const TractogramItem<ValueType> &item) {
  const auto &tck = item.streamline;
  enforce_vertices(tck, trk_vertex_tolerance);
  const size_t npoints = tck.size();
  const size_t per_vertex_floats = 3 + n_scalars;
  std::vector<std::byte> record(sizeof(int32_t) + npoints * per_vertex_floats * trk_value_bytes +
                                n_properties * trk_value_bytes);
  Raw::store_LE<int32_t>(static_cast<int32_t>(npoints), record.data(), 0);
  size_t offset = sizeof(int32_t);

  for (size_t v = 0; v != npoints; ++v) {
    const Eigen::Vector3d voxel = scanner2voxel * tck[v].template cast<double>();
    for (size_t axis = 0; axis != 3; ++axis) {
      const float mm = static_cast<float>(voxel[axis] * voxel_size[axis]);
      Raw::store_LE<float>(mm, record.data() + offset);
      offset += trk_value_bytes;
    }
    // Per-vertex scalars: each dpv field's M columns for vertex v, in field order.
    for (const auto &field : scalar_fields) {
      assert(field.ordinal < item.dpv.size());
      MR::match_v(item.dpv[field.ordinal], [&](const auto &value) {
        for (size_t c = 0; c != field.descriptor.columns; ++c) {
          const float scalar = static_cast<float>(value(static_cast<Eigen::Index>(v), static_cast<Eigen::Index>(c)));
          Raw::store_LE<float>(scalar, record.data() + offset);
          offset += trk_value_bytes;
        }
      });
    }
  }

  // Per-streamline properties: each dps field's M columns, in field order.
  for (const auto &field : property_fields) {
    assert(field.ordinal < item.dps.size());
    MR::match_v(item.dps[field.ordinal], [&](const auto &value) {
      for (size_t c = 0; c != field.descriptor.columns; ++c) {
        const float property = static_cast<float>(value(0, static_cast<Eigen::Index>(c)));
        Raw::store_LE<float>(property, record.data() + offset);
        offset += trk_value_bytes;
      }
    });
  }

  body_buffer.add(record.data(), record.size());
  ++num_streamlines;
  return true;
}

template <class ValueType> void TRKWriter<ValueType>::finalise() {
  body_buffer.commit();

  TRKUtils::Header header;
  std::memset(&header, 0, sizeof(header));
  std::memcpy(header.id_string.data(), "TRACK", 5);
  for (size_t axis = 0; axis != 3; ++axis) {
    header.dim[axis] = dim[axis];
    header.voxel_size[axis] = voxel_size[axis];
    header.origin[axis] = 0.0F;
  }
  header.n_scalars = static_cast<int16_t>(n_scalars);
  header.n_properties = static_cast<int16_t>(n_properties);

  // Name the scalar columns (dpv) and property columns (dps); an M>1 field's
  //   columns are suffixed so each named column is distinct within the 20-char
  //   limit (the TrackVis cap of 10 named columns was enforced at construction).
  size_t scalar_column = 0;
  for (const auto &field : scalar_fields) {
    for (size_t c = 0; c != field.descriptor.columns; ++c) {
      const std::string name =
          (field.descriptor.columns == 1) ? field.descriptor.name : field.descriptor.name + "_" + str(c);
      write_name(header.scalar_name[scalar_column++], name);
    }
  }
  size_t property_column = 0;
  for (const auto &field : property_fields) {
    for (size_t c = 0; c != field.descriptor.columns; ++c) {
      const std::string name =
          (field.descriptor.columns == 1) ? field.descriptor.name : field.descriptor.name + "_" + str(c);
      write_name(header.property_name[property_column++], name);
    }
  }

  for (size_t row = 0; row != 4; ++row)
    for (size_t col = 0; col != 4; ++col)
      header.vox_to_ras[row][col] = vox_to_ras[row][col];

  std::memcpy(header.voxel_order.data(), "LPS", 3);
  header.n_count = static_cast<int32_t>(num_streamlines);
  header.version = 2;
  header.hdr_size = TRKUtils::header_size_sentinel;

  // Step 5: stamp the provenance into "voxel_order"'s sibling reserved space. The
  //   spec has no dedicated software field, so the MRtrix3 command string (or the
  //   software name + version if it does not fit) is written into the otherwise
  //   unused 444-byte "reserved" block.
  const std::string command = App::command_history_string;
  const std::string provenance =
      (command.size() < header.reserved.size()) ? command : std::string("MRtrix3 ") + App::mrtrix_version;
  const size_t length = std::min(provenance.size(), header.reserved.size() - 1);
  std::memcpy(header.reserved.data(), provenance.data(), length);

  // Write the header, then concatenate the buffered body. The header sits at a
  //   fixed offset, so the n_count field is already final (no in-place patch
  //   needed): the header is simply written last.
  File::OFStream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
  out.write(reinterpret_cast<const char *>(&header), sizeof(header));
  {
    std::ifstream in(body_tempfile, std::ios::binary);
    if (in)
      out << in.rdbuf();
  }
  out.close();

  std::error_code ec;
  std::filesystem::remove(body_tempfile, ec);
}

template <class ValueType> TRKWriter<ValueType>::~TRKWriter() {
  try {
    finalise();
  } catch (Exception &e) {
    Exception(e, "TrackVis \".trk\" tractography file not properly finalised").display();
  }
}

/* ************************************************************************ */
/*               Explicit instantiation for float and double              */
/* ************************************************************************ */

template class TRKReader<float>;
template class TRKReader<double>;
template class TRKWriter<float>;
template class TRKWriter<double>;

namespace Formats {

bool TRK::handles(const std::filesystem::path &path) const { return path.extension() == ".trk"; }

std::unique_ptr<ReaderInterface<float>> TRK::read_float(const std::filesystem::path &path,
                                                        Properties &properties,
                                                        FieldRegistry &registry,
                                                        const OptionalHeader &grid) const {
  return std::make_unique<TRKReader<float>>(path, properties, registry, grid);
}

std::unique_ptr<ReaderInterface<double>> TRK::read_double(const std::filesystem::path &path,
                                                          Properties &properties,
                                                          FieldRegistry &registry,
                                                          const OptionalHeader &grid) const {
  return std::make_unique<TRKReader<double>>(path, properties, registry, grid);
}

std::unique_ptr<WriterInterface<float>> TRK::create_float(const std::filesystem::path &path,
                                                          const Properties &properties,
                                                          const FieldRegistry &registry,
                                                          const OptionalHeader &grid,
                                                          const WriteOptions &options) const {
  return std::make_unique<TRKWriter<float>>(path, properties, registry, grid);
}

std::unique_ptr<WriterInterface<double>> TRK::create_double(const std::filesystem::path &path,
                                                            const Properties &properties,
                                                            const FieldRegistry &registry,
                                                            const OptionalHeader &grid,
                                                            const WriteOptions &options) const {
  return std::make_unique<TRKWriter<double>>(path, properties, registry, grid);
}

} // namespace Formats

} // namespace MR::DWI::Tractography
