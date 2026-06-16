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

#include "dwi/tractography/formats/tck.h"

#include "dwi/tractography/nonfinite.h"

#include <array>
#include <cerrno>
#include <limits>

#include "app.h"
#include "exception.h"
#include "file/config.h"
#include "file/matrix.h"
#include "file/ofstream.h"
#include "half.h"

namespace MR::DWI::Tractography {

/* ************************************************************************ */
/*                          TCKReader<ValueType>                           */
/* ************************************************************************ */

template <class ValueType> TCKReader<ValueType>::TCKReader(const std::filesystem::path &path, Properties &properties) {
  open(path, "tracks", properties);
}

template <class ValueType> bool TCKReader<ValueType>::operator()(Streamline<ValueType> &tck) {
  tck.clear();

  if (!in.is_open())
    return false;

  do {
    auto p = get_next_point();
    if (std::isinf(p[0])) {
      in.close();
      return false;
    }
    if (in.eof()) {
      in.close();
      return false;
    }

    if (std::isnan(p[0])) {
      // The streamline weight (Streamline::weight, default 1.0) is populated by the
      //   framework-level weight loader (dwi/tractography/weights.h) when the user
      //   designates a source, never by this format reader.
      tck.set_index(current_index++);
      return true;
    }

    tck.push_back(p);
  } while (in.good());

  in.close();
  return false;
}

template <class ValueType> Eigen::Matrix<ValueType, 3, 1> TCKReader<ValueType>::get_next_point() {
  using namespace ByteOrder;
  switch (dtype()) {
  case DataType::Float16LE: {
    std::array<Eigen::half, 3> p{};
    in.read(reinterpret_cast<char *>(p.data()), sizeof(p));
    return {static_cast<ValueType>(LE(p[0])), static_cast<ValueType>(LE(p[1])), static_cast<ValueType>(LE(p[2]))};
  }
  case DataType::Float16BE: {
    std::array<Eigen::half, 3> p{};
    in.read(reinterpret_cast<char *>(p.data()), sizeof(p));
    return {static_cast<ValueType>(BE(p[0])), static_cast<ValueType>(BE(p[1])), static_cast<ValueType>(BE(p[2]))};
  }
  case DataType::Float32LE: {
    std::array<float, 3> p{};
    in.read(reinterpret_cast<char *>(p.data()), sizeof(p));
    return {static_cast<ValueType>(LE(p[0])), static_cast<ValueType>(LE(p[1])), static_cast<ValueType>(LE(p[2]))};
  }
  case DataType::Float32BE: {
    std::array<float, 3> p{};
    in.read(reinterpret_cast<char *>(p.data()), sizeof(p));
    return {static_cast<ValueType>(BE(p[0])), static_cast<ValueType>(BE(p[1])), static_cast<ValueType>(BE(p[2]))};
  }
  case DataType::Float64LE: {
    std::array<double, 3> p{};
    in.read(reinterpret_cast<char *>(p.data()), sizeof(p));
    return {static_cast<ValueType>(LE(p[0])), static_cast<ValueType>(LE(p[1])), static_cast<ValueType>(LE(p[2]))};
  }
  case DataType::Float64BE: {
    std::array<double, 3> p{};
    in.read(reinterpret_cast<char *>(p.data()), sizeof(p));
    return {static_cast<ValueType>(BE(p[0])), static_cast<ValueType>(BE(p[1])), static_cast<ValueType>(BE(p[2]))};
  }
  default:
    assert(0);
    break;
  }
  return Eigen::Matrix<ValueType, 3, 1>::Constant(std::numeric_limits<ValueType>::quiet_NaN());
}

/* ************************************************************************ */
/*                          TCKWriter<ValueType>                           */
/* ************************************************************************ */

template <typename ValueType>
TCKWriter<ValueType>::TCKWriter(const std::filesystem::path &path,
                                const Properties &properties,
                                std::optional<size_t> buffer_capacity)
    : WriterBase<ValueType>(path),
      buffer(buffer_capacity.value_or(File::Config::get_int("TrackWriterBufferSize", 16777216)), sizeof(vector_type)) {

  if (path.extension() != ".tck")
    throw Exception("output track files must use the .tck suffix");

  File::OFStream out;
  try {
    out.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
  } catch (Exception &e) {
    throw Exception(e, "Unable to create output track file");
  }

  const_cast<Properties &>(properties).set_timestamp();
  const_cast<Properties &>(properties).set_version_info();
  const_cast<Properties &>(properties).update_command_history();

  create(out, properties, "tracks");
  barrier_addr = out.tellp();

  vector_type x;
  format_point(barrier(), x);
  out.write(reinterpret_cast<const char *>(&x[0]), sizeof(x)); // check_syntax off
  if (!out.good())
    throw Exception("error writing tracks file \"" + path.string() + "\": " + MR::C_strerror(errno));
  open_success = true;

  // The .tck header count is patched from the live WriterBase counters inside
  //   flush_points() via update_counts(), so no separate count state is forwarded.
  buffer.set_flush_callback([this](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &counts) {
    this->flush_points(data, size, counts);
  });
}

template <class ValueType> void TCKWriter<ValueType>::format_point(const vector_type &src, vector_type &dest) {
  using namespace ByteOrder;
  if (dtype.is_little_endian())
    dest = {LE(src[0]), LE(src[1]), LE(src[2])};
  else
    dest = {BE(src[0]), BE(src[1]), BE(src[2])};
}

template <typename ValueType> TCKWriter<ValueType>::~TCKWriter() {
  try {
    commit();
  } catch (Exception &e) {
    Exception(e, "Tractography file not properly finalised").display();
  }
}

template <typename ValueType> bool TCKWriter<ValueType>::operator()(const Streamline<ValueType> &tck) {
  enforce_vertices(tck, tck_vertex_tolerance);
  for (const auto &i : tck)
    add_point(i);
  add_point(delimiter());

  ++count;
  ++total_count;
  return true;
}

template <typename ValueType>
void TCKWriter<ValueType>::flush_points(const std::byte *data,
                                        size_t size,
                                        const Formats::WriteBuffer::Counts & /*counts*/) {
  if (size == 0 || !this->open_success)
    return;

  // The .tck binary stream is terminated by an Inf "barrier" point; appending a
  //   new batch overwrites the previous barrier with the incoming points and
  //   writes a fresh barrier at the new end of the data region.
  const int64_t prev_barrier_addr = this->barrier_addr;
  vector_type formatted_barrier;
  format_point(this->barrier(), formatted_barrier);

  File::OFStream out(this->path, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
  out.seekp(prev_barrier_addr, out.beg);
  out.write(reinterpret_cast<const char *>(data), size);
  this->verify_stream(out);
  out.write(reinterpret_cast<const char *>(&formatted_barrier[0]), sizeof(vector_type));
  this->verify_stream(out);
  this->barrier_addr = static_cast<int64_t>(out.tellp()) - sizeof(vector_type);
  this->update_counts(out);
}

template <typename ValueType> void TCKWriter<ValueType>::commit() { buffer.commit(); }

/* ************************************************************************ */
/*               Explicit instantiation for float and double              */
/* ************************************************************************ */

template class TCKReader<float>;
template class TCKReader<double>;
template class TCKWriter<float>;
template class TCKWriter<double>;

namespace Formats {

bool TCK::handles(const std::filesystem::path &path) const { return path.extension() == ".tck"; }

std::unique_ptr<ReaderInterface<float>> TCK::read_float(const std::filesystem::path &path,
                                                        Properties &properties,
                                                        FieldRegistry &,
                                                        const OptionalHeader &) const {
  return std::make_unique<TCKReader<float>>(path, properties);
}

std::unique_ptr<ReaderInterface<double>> TCK::read_double(const std::filesystem::path &path,
                                                          Properties &properties,
                                                          FieldRegistry &,
                                                          const OptionalHeader &) const {
  return std::make_unique<TCKReader<double>>(path, properties);
}

std::unique_ptr<WriterInterface<float>> TCK::create_float(const std::filesystem::path &path,
                                                          const Properties &properties,
                                                          const FieldRegistry &,
                                                          const OptionalHeader &,
                                                          const WriteOptions &options) const {
  return std::make_unique<TCKWriter<float>>(path, properties, options.buffer_capacity);
}

std::unique_ptr<WriterInterface<double>> TCK::create_double(const std::filesystem::path &path,
                                                            const Properties &properties,
                                                            const FieldRegistry &,
                                                            const OptionalHeader &,
                                                            const WriteOptions &options) const {
  return std::make_unique<TCKWriter<double>>(path, properties, options.buffer_capacity);
}

} // namespace Formats

} // namespace MR::DWI::Tractography
