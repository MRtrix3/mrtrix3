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

#include <array>
#include <cerrno>
#include <limits>

#include "app.h"
#include "exception.h"
#include "file/config.h"
#include "file/matrix.h"
#include "file/ofstream.h"

namespace MR::DWI::Tractography {

/* ************************************************************************ */
/*                          Reader<ValueType>                              */
/* ************************************************************************ */

template <class ValueType> Reader<ValueType>::Reader(const std::filesystem::path &path, Properties &properties) {
  open(path, "tracks", properties);
  auto opt = App::get_options("tck_weights_in");
  if (!opt.empty())
    weights = File::Matrix::load_vector<ValueType>(opt[0][0]);
}

template <class ValueType> bool Reader<ValueType>::operator()(Streamline<ValueType> &tck) {
  tck.clear();

  if (!in.is_open())
    return false;

  do {
    auto p = get_next_point();
    if (std::isinf(p[0])) {
      in.close();
      check_excess_weights();
      return false;
    }
    if (in.eof()) {
      in.close();
      check_excess_weights();
      return false;
    }

    if (std::isnan(p[0])) {
      tck.set_index(current_index++);

      if (weights.size()) {

        if (tck.get_index() < static_cast<size_t>(weights.size())) {
          tck.weight = weights[tck.get_index()];
        } else {
          WARN("Streamline weights file contains less entries (" + str(weights.size()) +
               ") than .tck file; "
               "ceasing reading of streamline data");
          in.close();
          tck.clear();
          return false;
        }

      } else {
        tck.weight = 1.0;
      }

      return true;
    }

    tck.push_back(p);
  } while (in.good());

  in.close();
  return false;
}

template <class ValueType> Eigen::Matrix<ValueType, 3, 1> Reader<ValueType>::get_next_point() {
  using namespace ByteOrder;
  switch (dtype()) {
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

template <class ValueType> void Reader<ValueType>::check_excess_weights() {
  if (!weights.size())
    return;
  if (static_cast<size_t>(weights.size()) > current_index) {
    WARN("Streamline weights file contains more entries (" + str(weights.size()) + ") than .tck file (" +
         str(current_index) + ")");
  }
}

/* ************************************************************************ */
/*                       WriterUnbuffered<ValueType>                       */
/* ************************************************************************ */

template <class ValueType>
WriterUnbuffered<ValueType>::WriterUnbuffered(const std::filesystem::path &path, const Properties &properties)
    : WriterBase<ValueType>(path) {

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

  auto opt = App::get_options("tck_weights_out");
  if (!opt.empty())
    set_weights_path(opt[0][0]);
}

template <class ValueType> bool WriterUnbuffered<ValueType>::operator()(const Streamline<ValueType> &tck) {
  // allocate buffer on the stack for performance:
  NON_POD_VLA(buffer, vector_type, tck.size() + 2);
  for (size_t n = 0; n < tck.size(); ++n) {
    assert(tck[n].allFinite());
    format_point(tck[n], buffer[n]);
  }
  format_point(delimiter(), buffer[tck.size()]);

  commit(buffer, tck.size() + 1);

  if (!weights_path.empty())
    write_weights(str(tck.weight) + "\n");

  ++count;
  ++total_count;
  return true;
}

template <class ValueType> void WriterUnbuffered<ValueType>::set_weights_path(const std::filesystem::path &path) {
  if (!weights_path.empty())
    throw Exception("Cannot change output streamline weights file path");
  weights_path = path;
  App::check_overwrite(weights_path);
  File::OFStream out(weights_path, std::ios::out | std::ios::binary | std::ios::trunc);
}

template <class ValueType> void WriterUnbuffered<ValueType>::format_point(const vector_type &src, vector_type &dest) {
  using namespace ByteOrder;
  if (dtype.is_little_endian())
    dest = {LE(src[0]), LE(src[1]), LE(src[2])};
  else
    dest = {BE(src[0]), BE(src[1]), BE(src[2])};
}

template <class ValueType> void WriterUnbuffered<ValueType>::write_weights(std::string_view contents) {
  File::OFStream out(weights_path, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
  out << contents;
  if (!out.good())
    throw Exception("error writing streamline weights file \"" + weights_path.string() + "\": " + //
                    MR::C_strerror(errno));
}

template <class ValueType> void WriterUnbuffered<ValueType>::commit(vector_type *data, size_t num_points) {
  if (num_points == 0 || !open_success)
    return;

  int64_t prev_barrier_addr = barrier_addr;

  format_point(barrier(), data[num_points]);
  File::OFStream out(path, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
  out.write(reinterpret_cast<const char *>(data + 1), sizeof(vector_type) * num_points);
  verify_stream(out);
  barrier_addr = static_cast<int64_t>(out.tellp()) - sizeof(vector_type);
  out.seekp(prev_barrier_addr, out.beg);
  out.write(reinterpret_cast<const char *>(data), sizeof(vector_type));
  verify_stream(out);
  update_counts(out);
}

/* ************************************************************************ */
/*                            Writer<ValueType>                            */
/* ************************************************************************ */

template <typename ValueType>
Writer<ValueType>::Writer(const std::filesystem::path &path,
                          const Properties &properties,
                          size_t default_buffer_capacity)
    : WriterUnbuffered<ValueType>(path, properties),
      buffer(File::Config::get_int("TrackWriterBufferSize", default_buffer_capacity), sizeof(vector_type)) {
  // The .tck header count is patched from the live WriterBase counters inside
  //   flush_points() via update_counts(), so no separate count state is forwarded.
  buffer.set_flush_callback([this](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &counts) {
    this->flush_points(data, size, counts);
  });
}

template <typename ValueType> Writer<ValueType>::~Writer() {
  try {
    commit();
  } catch (Exception &e) {
    Exception(e, "Tractography file not properly finalised").display();
  }
}

template <typename ValueType> bool Writer<ValueType>::operator()(const Streamline<ValueType> &tck) {
  for (const auto &i : tck) {
    assert(i.allFinite());
    add_point(i);
  }
  add_point(delimiter());

  if (!weights_path.empty())
    weights_buffer += str(tck.weight) + ' ';

  ++count;
  ++total_count;
  return true;
}

template <typename ValueType>
void Writer<ValueType>::flush_points(const std::byte *data,
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

template <typename ValueType> void Writer<ValueType>::commit() {
  buffer.commit();

  if (!weights_path.empty()) {
    write_weights(weights_buffer);
    weights_buffer.clear();
  }
}

/* ************************************************************************ */
/*               Explicit instantiation for float and double              */
/* ************************************************************************ */

template class Reader<float>;
template class Reader<double>;
template class WriterUnbuffered<float>;
template class WriterUnbuffered<double>;
template class Writer<float>;
template class Writer<double>;

namespace Formats {

bool TCK::handles(const std::filesystem::path &path) const { return path.extension() == ".tck"; }

std::unique_ptr<ReaderInterface<float>> TCK::read_float(const std::filesystem::path &path,
                                                        Properties &properties) const {
  return std::make_unique<Reader<float>>(path, properties);
}

std::unique_ptr<ReaderInterface<double>> TCK::read_double(const std::filesystem::path &path,
                                                          Properties &properties) const {
  return std::make_unique<Reader<double>>(path, properties);
}

std::unique_ptr<WriterInterface<float>> TCK::create_float(const std::filesystem::path &path,
                                                          const Properties &properties) const {
  return std::make_unique<Writer<float>>(path, properties);
}

std::unique_ptr<WriterInterface<double>> TCK::create_double(const std::filesystem::path &path,
                                                            const Properties &properties) const {
  return std::make_unique<Writer<double>>(path, properties);
}

} // namespace Formats

} // namespace MR::DWI::Tractography
