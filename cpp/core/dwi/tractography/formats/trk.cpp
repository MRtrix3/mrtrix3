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

//! \brief write a std::string into a fixed-width NUL-padded header name field.
void write_name(std::array<char, TRKUtils::name_length> &field, std::string_view name) {
  field.fill('\0');
  const size_t length = std::min(name.size(), TRKUtils::name_length);
  std::memcpy(field.data(), name.data(), length);
}

} // namespace

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

  // Provenance (the MRtrix3 command string) is stamped into the "reserved" block
  //   in step 5; left unset here.

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

template class TRKWriter<float>;
template class TRKWriter<double>;

} // namespace MR::DWI::Tractography
