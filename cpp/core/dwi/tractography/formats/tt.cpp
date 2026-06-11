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

#include "dwi/tractography/formats/tt.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

#include "app.h"
#include "exception.h"
#include "file/config.h"
#include "file/gz.h"
#include "file/ofstream.h"
#include "file/temp.h"
#include "header.h"
#include "raw.h"

#include "dwi/tractography/formats/mat.h"

namespace MR::DWI::Tractography {

//! \brief Convenience alias for the shared Level-5 ".mat" container utilities.
namespace Mat = Formats::Mat;

namespace {

//! the on-disk coordinate resolution of ".tt": coordinates are in 1/32-voxel units
constexpr double tt_subvoxel_scale = 32.0;
//! the fixed ".tt" per-streamline record header: uint32 length + 3 x int32 first vertex
constexpr size_t tt_record_header_bytes = 16;

//! \brief gzip-inflate an entire file into a byte buffer.
/*! The ".tt" container is a single gzip stream; zlib's gzread caps a single
 * call to a 32-bit count, so the whole stream is drained in chunks. */
std::vector<std::byte> gz_inflate(const std::filesystem::path &path) {
  File::GZ gz(path, "rb");
  std::vector<std::byte> out;
  constexpr size_t chunk = 1 << 20; // 1 MB per read
  size_t filled = 0;
  while (true) {
    out.resize(filled + chunk);
    const int n = gz.read(out.data() + filled, chunk);
    if (n < 0)
      throw Exception("error inflating \".tt\" file \"" + path.string() + "\"");
    filled += static_cast<size_t>(n);
    if (static_cast<size_t>(n) < chunk)
      break;
  }
  out.resize(filled);
  return out;
}

//! \brief construct the default axis-aligned voxel->scanner transform from spacing.
/*! Used when no reference Header is available: an identity rotation with the
 * grid corner at the scanner-space origin, scaled by the voxel spacing. Geometry
 * is then correct only up to the unknown orientation/origin of the acquisition,
 * but a ".tt" -> ".tt" round-trip through this same convention is exact. */
transform_type default_voxel2scanner(const std::array<float, 3> &voxel_size) {
  transform_type T;
  T.setIdentity();
  for (size_t axis = 0; axis != 3; ++axis)
    T(axis, axis) = voxel_size[axis];
  return T;
}

//! \brief read a numeric ".mat" member into three doubles, regardless of its element class.
/*! "dimension" / "voxel_size" may be stored in any numeric class by the
 * producer; the values are widened to double for grid construction. */
std::array<double, 3> read_triple(const Mat::Array &array, std::string_view name) {
  if (array.size() < 3)
    throw Exception("\".tt\" member \"" + std::string(name) + "\" has fewer than 3 elements");
  std::array<double, 3> result{};
  switch (array.cls) {
  case Mat::Class::Int8:
    for (size_t i = 0; i != 3; ++i)
      result[i] = array.as<int8_t>()[i];
    break;
  case Mat::Class::UInt8:
    for (size_t i = 0; i != 3; ++i)
      result[i] = array.as<uint8_t>()[i];
    break;
  case Mat::Class::Int16:
    for (size_t i = 0; i != 3; ++i)
      result[i] = array.as<int16_t>()[i];
    break;
  case Mat::Class::UInt16:
    for (size_t i = 0; i != 3; ++i)
      result[i] = array.as<uint16_t>()[i];
    break;
  case Mat::Class::Int32:
    for (size_t i = 0; i != 3; ++i)
      result[i] = array.as<int32_t>()[i];
    break;
  case Mat::Class::UInt32:
    for (size_t i = 0; i != 3; ++i)
      result[i] = array.as<uint32_t>()[i];
    break;
  case Mat::Class::Single:
    for (size_t i = 0; i != 3; ++i)
      result[i] = array.as<float>()[i];
    break;
  case Mat::Class::Double:
    for (size_t i = 0; i != 3; ++i)
      result[i] = array.as<double>()[i];
    break;
  default:
    throw Exception("\".tt\" member \"" + std::string(name) + "\" has an unsupported numeric class");
  }
  return result;
}

} // namespace

/* ************************************************************************ */
/*                           TTReader<ValueType>                           */
/* ************************************************************************ */

template <class ValueType>
TTReader<ValueType>::TTReader(const std::filesystem::path &path, Properties &properties, const OptionalHeader &grid)
    : position(0), current_index(0) {
  const std::vector<std::byte> bytes = gz_inflate(path);
  const Mat::File mat = Mat::read(bytes.data(), bytes.size());

  const Mat::Array *const dimension = mat.find("dimension");
  const Mat::Array *const voxel = mat.find("voxel_size");
  const Mat::Array *const track_member = mat.find("track");
  if (track_member == nullptr)
    throw Exception("\".tt\" file \"" + path.string() + "\" contains no \"track\" member");

  std::array<float, 3> voxel_size{1.0F, 1.0F, 1.0F};
  if (voxel != nullptr) {
    const std::array<double, 3> v = read_triple(*voxel, "voxel_size");
    for (size_t axis = 0; axis != 3; ++axis)
      voxel_size[axis] = static_cast<float>(v[axis]);
  }

  // The grid orientation/origin is not carried by ".tt"; use the supplied
  //   reference Header's voxel->scanner transform when available, else a default
  //   axis-aligned grid built from the file's own spacing.
  if (grid.has_value()) {
    voxel2scanner = Transform(grid->get()).voxel2scanner;
  } else {
    voxel2scanner = default_voxel2scanner(voxel_size);
  }

  // Preserve the grid metadata and opaque provenance into Properties.
  if (dimension != nullptr) {
    const std::array<double, 3> d = read_triple(*dimension, "dimension");
    properties["tt_dimension"] = str(static_cast<int64_t>(std::lround(d[0]))) + "," +
                                 str(static_cast<int64_t>(std::lround(d[1]))) + "," +
                                 str(static_cast<int64_t>(std::lround(d[2])));
  }
  properties["tt_voxel_size"] = str(voxel_size[0]) + "," + str(voxel_size[1]) + "," + str(voxel_size[2]);
  if (const Mat::Array *const parameter_id = mat.find("parameter_id"))
    properties["tt_parameter_id"] = parameter_id->as_string();
  if (const Mat::Array *const report = mat.find("report")) {
    const std::string text = report->as_string();
    if (!text.empty())
      properties.comments.push_back(text);
  }

  // The "track" member is a flat byte buffer regardless of its int8/uint8 class.
  track.assign(track_member->data.begin(), track_member->data.end());
}

template <class ValueType> bool TTReader<ValueType>::operator()(Streamline<ValueType> &tck) {
  tck.clear();
  if (position + tt_record_header_bytes > track.size())
    return false;

  const std::byte *const base = track.data();
  const uint32_t length = Raw::fetch_LE<uint32_t>(base + position);
  // length == 3 * npoints; the record occupies length + 13 bytes total
  //   (16-byte header + 3*(npoints-1) delta bytes).
  if (length % 3 != 0)
    throw Exception("malformed \".tt\" record: vertex-count field (" + str(length) + ") is not a multiple of 3");
  const size_t npoints = length / 3;
  const size_t record_bytes = tt_record_header_bytes + (npoints > 0 ? 3 * (npoints - 1) : 0);
  if (position + record_bytes > track.size())
    throw Exception("malformed \".tt\" file: streamline record overruns the \"track\" buffer");

  int32_t ix = Raw::fetch_LE<int32_t>(base + position + 4);
  int32_t iy = Raw::fetch_LE<int32_t>(base + position + 8);
  int32_t iz = Raw::fetch_LE<int32_t>(base + position + 12);

  const std::byte *deltas = base + position + tt_record_header_bytes;
  tck.set_index(current_index++);
  for (size_t j = 0; j != npoints; ++j) {
    if (j != 0) {
      ix += static_cast<int32_t>(static_cast<int8_t>(deltas[0]));
      iy += static_cast<int32_t>(static_cast<int8_t>(deltas[1]));
      iz += static_cast<int32_t>(static_cast<int8_t>(deltas[2]));
      deltas += 3;
    }
    const Eigen::Vector3d voxel(ix / tt_subvoxel_scale, iy / tt_subvoxel_scale, iz / tt_subvoxel_scale);
    const Eigen::Vector3d scanner = voxel2scanner * voxel;
    tck.push_back(scanner.cast<ValueType>());
  }

  position += record_bytes;
  return true;
}

/* ************************************************************************ */
/*                           TTWriter<ValueType>                           */
/* ************************************************************************ */

template <class ValueType>
TTWriter<ValueType>::TTWriter(const std::filesystem::path &path,
                              const Properties &properties,
                              const OptionalHeader &grid)
    : path(path),
      dimension{0, 0, 0},
      voxel_size{1.0F, 1.0F, 1.0F},
      grid_from_reference(false),
      track_buffer(File::Config::get_int("TrackWriterBufferSize", 16777216), 1),
      num_streamlines(0) {
  if (path.extension() != ".tt")
    throw Exception("output DSI Studio track files must use the .tt suffix");

  App::check_overwrite(path);

  if (grid.has_value()) {
    const Header &header = grid->get();
    scanner2voxel = Transform(header).scanner2voxel;
    for (size_t axis = 0; axis != 3; ++axis) {
      dimension[axis] = static_cast<int32_t>(header.size(axis));
      voxel_size[axis] = static_cast<float>(header.spacing(axis));
    }
    grid_from_reference = true;
  } else {
    // No reference grid: recover spacing from any "tt_voxel_size" property left by
    //   a prior ".tt" read (loss-free ".tt" -> ".tt"), else default to 1 mm
    //   isotropic. The default voxel->scanner is axis-aligned with a corner
    //   origin, so its inverse maps scanner-space straight back to voxel space.
    auto vs = properties.find("tt_voxel_size");
    if (vs != properties.end()) {
      const auto values = MR::parse_floats(vs->second);
      for (size_t axis = 0; axis != 3 && axis != values.size(); ++axis)
        voxel_size[axis] = static_cast<float>(values[axis]);
    }
    scanner2voxel = default_voxel2scanner(voxel_size).inverse();
    auto dim = properties.find("tt_dimension");
    if (dim != properties.end()) {
      const auto values = MR::parse_ints<int64_t>(dim->second);
      for (size_t axis = 0; axis != 3 && axis != values.size(); ++axis)
        dimension[axis] = static_cast<int32_t>(values[axis]);
    }
  }

  // Opaque provenance carried back out verbatim if it survived a prior ".tt" read.
  auto pid = properties.find("tt_parameter_id");
  if (pid != properties.end())
    parameter_id = pid->second;

  track_tempfile = File::create_tempfile(0, ".tttrack");
  const std::filesystem::path track_path = track_tempfile;
  track_buffer.set_flush_callback(
      [track_path](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &) {
        File::OFStream out(track_path, std::ios::out | std::ios::binary | std::ios::app);
        out.write(reinterpret_cast<const char *>(data), size);
      });
}

template <class ValueType> bool TTWriter<ValueType>::operator()(const Streamline<ValueType> &tck) {
  if (tck.empty())
    return true;

  const size_t npoints = tck.size();
  const uint32_t length = static_cast<uint32_t>(3 * npoints);

  // Quantise the first vertex (absolute) and integrate signed int8 deltas for the
  //   remaining vertices, in 1/32-voxel integer units.
  std::vector<std::byte> record(tt_record_header_bytes + (npoints > 0 ? 3 * (npoints - 1) : 0));
  Raw::store_LE<uint32_t>(length, record.data(), 0);

  auto quantise = [&](const Eigen::Matrix<ValueType, 3, 1> &pos) {
    const Eigen::Vector3d voxel = scanner2voxel * pos.template cast<double>();
    std::array<int32_t, 3> q{};
    for (size_t axis = 0; axis != 3; ++axis)
      q[axis] = static_cast<int32_t>(std::lround(voxel[axis] * tt_subvoxel_scale));
    return q;
  };

  std::array<int32_t, 3> prev = quantise(tck[0]);
  Raw::store_LE<int32_t>(prev[0], record.data() + 4, 0);
  Raw::store_LE<int32_t>(prev[1], record.data() + 4, 1);
  Raw::store_LE<int32_t>(prev[2], record.data() + 4, 2);

  // Expand grid extent from the data when no reference grid fixed it, so the
  //   written "dimension" comfortably contains the streamlines.
  if (!grid_from_reference) {
    for (size_t axis = 0; axis != 3; ++axis) {
      const int32_t voxel_index =
          static_cast<int32_t>(std::ceil(static_cast<double>(prev[axis]) / tt_subvoxel_scale)) + 1;
      dimension[axis] = std::max(dimension[axis], voxel_index);
    }
  }

  std::byte *delta_out = record.data() + tt_record_header_bytes;
  for (size_t j = 1; j != npoints; ++j) {
    const std::array<int32_t, 3> cur = quantise(tck[j]);
    for (size_t axis = 0; axis != 3; ++axis) {
      int32_t d = cur[axis] - prev[axis];
      // The on-disk delta is a signed int8; a step exceeding +/-127/32 voxels per
      //   axis cannot be represented losslessly, so clamp and advance the running
      //   accumulator by the clamped amount (matching how DSI Studio decodes).
      d = std::max<int32_t>(std::numeric_limits<int8_t>::min(),
                            std::min<int32_t>(std::numeric_limits<int8_t>::max(), d));
      *delta_out++ = static_cast<std::byte>(static_cast<int8_t>(d));
      prev[axis] += d;
    }
  }

  track_buffer.add(record.data(), record.size());
  ++num_streamlines;
  return true;
}

template <class ValueType> void TTWriter<ValueType>::finalise() {
  track_buffer.commit();

  // Slurp the accumulated track payload back from the temporary file.
  std::vector<std::byte> track_payload;
  {
    std::ifstream in(track_tempfile, std::ios::binary | std::ios::ate);
    if (in) {
      const std::streamsize size = in.tellg();
      in.seekg(0);
      track_payload.resize(static_cast<size_t>(std::max<std::streamsize>(0, size)));
      if (!track_payload.empty())
        in.read(reinterpret_cast<char *>(track_payload.data()), size);
    }
  }

  // Build the ".mat" container with the five named members DSI Studio expects.
  Mat::File mat;

  Mat::Array dimension_member;
  dimension_member.cls = Mat::Class::Int32;
  dimension_member.rows = 1;
  dimension_member.cols = 3;
  dimension_member.data.resize(3 * sizeof(int32_t));
  for (size_t axis = 0; axis != 3; ++axis)
    Raw::store_LE<int32_t>(std::max<int32_t>(1, dimension[axis]), dimension_member.data.data(), axis);
  mat.add("dimension", std::move(dimension_member));

  Mat::Array voxel_member;
  voxel_member.cls = Mat::Class::Single;
  voxel_member.rows = 1;
  voxel_member.cols = 3;
  voxel_member.data.resize(3 * sizeof(float));
  for (size_t axis = 0; axis != 3; ++axis)
    Raw::store_LE<float>(voxel_size[axis], voxel_member.data.data(), axis);
  mat.add("voxel_size", std::move(voxel_member));

  if (!parameter_id.empty()) {
    Mat::Array pid;
    pid.cls = Mat::Class::Char;
    pid.rows = 1;
    pid.cols = parameter_id.size();
    pid.data.assign(reinterpret_cast<const std::byte *>(parameter_id.data()),
                    reinterpret_cast<const std::byte *>(parameter_id.data()) + parameter_id.size());
    mat.add("parameter_id", std::move(pid));
  }

  if (!report.empty()) {
    Mat::Array rep;
    rep.cls = Mat::Class::Char;
    rep.rows = 1;
    rep.cols = report.size();
    rep.data.assign(reinterpret_cast<const std::byte *>(report.data()),
                    reinterpret_cast<const std::byte *>(report.data()) + report.size());
    mat.add("report", std::move(rep));
  }

  Mat::Array track_member;
  track_member.cls = Mat::Class::Int8;
  track_member.rows = track_payload.empty() ? 0 : 1;
  track_member.cols = track_payload.size();
  track_member.data = std::move(track_payload);
  mat.add("track", std::move(track_member));

  const std::vector<std::byte> mat_bytes = Mat::write(mat);

  // gzip-compress the ".mat" container into the output ".tt" file. File::GZ
  //   requires the target to already exist, so create it first.
  { File::OFStream create(path, std::ios::out | std::ios::binary | std::ios::trunc); }
  File::GZ gz(path, "wb");
  gz.write(mat_bytes.data(), mat_bytes.size());
  gz.close();

  std::error_code ec;
  std::filesystem::remove(track_tempfile, ec);
}

template <class ValueType> TTWriter<ValueType>::~TTWriter() {
  try {
    finalise();
  } catch (Exception &e) {
    Exception(e, "DSI Studio \".tt\" tractography file not properly finalised").display();
  }
}

/* ************************************************************************ */
/*               Explicit instantiation for float and double              */
/* ************************************************************************ */

template class TTReader<float>;
template class TTReader<double>;
template class TTWriter<float>;
template class TTWriter<double>;

namespace Formats {

bool TT::handles(const std::filesystem::path &path) const { return path.extension() == ".tt"; }

std::unique_ptr<ReaderInterface<float>> TT::read_float(const std::filesystem::path &path,
                                                       Properties &properties,
                                                       FieldRegistry &,
                                                       const OptionalHeader &grid) const {
  return std::make_unique<TTReader<float>>(path, properties, grid);
}

std::unique_ptr<ReaderInterface<double>> TT::read_double(const std::filesystem::path &path,
                                                         Properties &properties,
                                                         FieldRegistry &,
                                                         const OptionalHeader &grid) const {
  return std::make_unique<TTReader<double>>(path, properties, grid);
}

std::unique_ptr<WriterInterface<float>> TT::create_float(const std::filesystem::path &path,
                                                         const Properties &properties,
                                                         const FieldRegistry &,
                                                         const OptionalHeader &grid) const {
  return std::make_unique<TTWriter<float>>(path, properties, grid);
}

std::unique_ptr<WriterInterface<double>> TT::create_double(const std::filesystem::path &path,
                                                           const Properties &properties,
                                                           const FieldRegistry &,
                                                           const OptionalHeader &grid) const {
  return std::make_unique<TTWriter<double>>(path, properties, grid);
}

} // namespace Formats

} // namespace MR::DWI::Tractography
