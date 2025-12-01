/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#pragma once

#include <array>
#include <map>

#include "app.h"
#include "dwi/tractography/file_base.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"
#include "file/config.h"
#include "file/key_value.h"
#include "file/matrix.h"
#include "file/ofstream.h"
#include "memory.h"
#include "types.h"

namespace MR::DWI::Tractography {

template <class ValueType> class ReaderInterface {
public:
  virtual bool operator()(Streamline<ValueType> &) = 0;
  virtual ~ReaderInterface() {}
};

template <class ValueType> class WriterInterface {
public:
  virtual bool operator()(const Streamline<ValueType> &) = 0;
  virtual ~WriterInterface() {}
};

//! A class to read streamlines data
template <class ValueType = float> class Reader : public __ReaderBase__, public ReaderInterface<ValueType> {
public:
  //! open the \c file for reading and load header into \c properties
  Reader(std::string_view file, Properties &properties) {
    open(file, "tracks", properties);
    auto opt = App::get_options("tck_weights_in");
    if (!opt.empty())
      weights = File::Matrix::load_vector<ValueType>(opt[0][0]);
  }

  //! fetch next track from file
  bool operator()(Streamline<ValueType> &tck) {
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

protected:
  using __ReaderBase__::current_index;
  using __ReaderBase__::dtype;
  using __ReaderBase__::in;

  Eigen::Matrix<ValueType, Eigen::Dynamic, 1> weights;

  //! takes care of byte ordering issues

  Eigen::Matrix<ValueType, 3, 1> get_next_point() {
    using namespace ByteOrder;
    switch (dtype()) {
    case DataType::Float32LE: {
      std::array<float, 3> p;
      in.read(reinterpret_cast<char *>(p.data()), sizeof(p));
      return {static_cast<ValueType>(LE(p[0])), static_cast<ValueType>(LE(p[1])), static_cast<ValueType>(LE(p[2]))};
    }
    case DataType::Float32BE: {
      std::array<float, 3> p;
      in.read(reinterpret_cast<char *>(p.data()), sizeof(p));
      return {static_cast<ValueType>(BE(p[0])), static_cast<ValueType>(BE(p[1])), static_cast<ValueType>(BE(p[2]))};
    }
    case DataType::Float64LE: {
      std::array<double, 3> p;
      in.read(reinterpret_cast<char *>(p.data()), sizeof(p));
      return {static_cast<ValueType>(LE(p[0])), static_cast<ValueType>(LE(p[1])), static_cast<ValueType>(LE(p[2]))};
    }
    case DataType::Float64BE: {
      std::array<double, 3> p;
      in.read(reinterpret_cast<char *>(p.data()), sizeof(p));
      return {static_cast<ValueType>(BE(p[0])), static_cast<ValueType>(BE(p[1])), static_cast<ValueType>(BE(p[2]))};
    }
    default:
      assert(0);
      break;
    }
    return Eigen::Matrix<ValueType, 3, 1>::Constant(std::numeric_limits<ValueType>::quiet_NaN());
  }

  //! Check that the weights file does not contain excess entries
  void check_excess_weights() {
    if (!weights.size())
      return;
    if (static_cast<size_t>(weights.size()) > current_index) {
      WARN("Streamline weights file contains more entries (" + str(weights.size()) + ") than .tck file (" +
           str(current_index) + ")");
    }
  }

  Reader(const Reader &) = delete;
};

//! class to handle unbuffered writing of tracks to file
/*! writes track header as specified in \a properties and individual
 * tracks to the file specified in \a file. Writing individual tracks is
 * done using the operator() method.
 *
 * This class re-opens the output file every time a new streamline is
 * written. This may result in slow operation in some circumstances, and
 * may lead to fragmentation on some file systems, but is necessary in
 * use cases where a very large number of track files are being written
 * at once. For most applications (where typically one track file is
 * written at a time), the Writer class is more appropriate.
 * */
template <class ValueType = float>
class WriterUnbuffered : public __WriterBase__<ValueType>, public WriterInterface<ValueType> {
public:
  using __WriterBase__<ValueType>::count;
  using __WriterBase__<ValueType>::total_count;
  using __WriterBase__<ValueType>::name;
  using __WriterBase__<ValueType>::dtype;
  using __WriterBase__<ValueType>::create;
  using __WriterBase__<ValueType>::verify_stream;
  using __WriterBase__<ValueType>::update_counts;
  using __WriterBase__<ValueType>::open_success;

  using vector_type = Eigen::Matrix<ValueType, 3, 1>;

  //! create a new track file with the specified properties
  WriterUnbuffered(std::string_view file, const Properties &properties) : __WriterBase__<ValueType>(file) {

    if (!Path::has_suffix(name, ".tck"))
      throw Exception("output track files must use the .tck suffix");

    File::OFStream out;
    try {
      out.open(name, std::ios::out | std::ios::binary | std::ios::trunc);
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
    out.write(reinterpret_cast<const char *>(&x[0]), sizeof(x));
    if (!out.good())
      throw Exception("error writing tracks file \"" + name + "\": " + strerror(errno));
    open_success = true;

    auto opt = App::get_options("tck_weights_out");
    if (!opt.empty())
      set_weights_path(opt[0][0]);
  }

  //! append track to file
  bool operator()(const Streamline<ValueType> &tck) {
    // allocate buffer on the stack for performance:
    NON_POD_VLA(buffer, vector_type, tck.size() + 2);
    for (size_t n = 0; n < tck.size(); ++n) {
      assert(tck[n].allFinite());
      format_point(tck[n], buffer[n]);
    }
    format_point(delimiter(), buffer[tck.size()]);

    commit(buffer, tck.size() + 1);

    if (!weights_name.empty())
      write_weights(str(tck.weight) + "\n");

    ++count;
    ++total_count;
    return true;
  }

  //! set the path to the track weights
  void set_weights_path(std::string_view path) {
    if (!weights_name.empty())
      throw Exception("Cannot change output streamline weights file path");
    weights_name = path;
    App::check_overwrite(weights_name);
    File::OFStream out(weights_name, std::ios::out | std::ios::binary | std::ios::trunc);
  }

protected:
  std::string weights_name;
  int64_t barrier_addr;

  //! indicates end of track and start of new track
  vector_type delimiter() const { return vector_type::Constant(std::numeric_limits<ValueType>::quiet_NaN()); }
  //! indicates end of data
  vector_type barrier() const { return vector_type::Constant(std::numeric_limits<ValueType>::infinity()); }

  //! perform per-point byte-swapping if required
  void format_point(const vector_type &src, vector_type &dest) {
    using namespace ByteOrder;
    if (dtype.is_little_endian())
      dest = {LE(src[0]), LE(src[1]), LE(src[2])};
    else
      dest = {BE(src[0]), BE(src[1]), BE(src[2])};
  }

  //! write track weights data to file
  void write_weights(std::string_view contents) {
    File::OFStream out(weights_name, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
    out << contents;
    if (!out.good())
      throw Exception("error writing streamline weights file \"" + weights_name + "\": " + strerror(errno));
  }

  //! write track point data to file
  /*! \note \c buffer needs to be greater than \c num_points by one
   * element to add the barrier. */
  void commit(vector_type *data, size_t num_points) {
    if (num_points == 0 || !open_success)
      return;

    int64_t prev_barrier_addr = barrier_addr;

    format_point(barrier(), data[num_points]);
    File::OFStream out(name, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
    out.write(reinterpret_cast<const char *>(data + 1), sizeof(vector_type) * num_points);
    verify_stream(out);
    barrier_addr = static_cast<int64_t>(out.tellp()) - sizeof(vector_type);
    out.seekp(prev_barrier_addr, out.beg);
    out.write(reinterpret_cast<const char *>(data), sizeof(vector_type));
    verify_stream(out);
    update_counts(out);
  }

  //! copy construction explicitly disabled
  WriterUnbuffered(const WriterUnbuffered &) = delete;
};

//! class to handle writing tracks to file, with RAM buffer
/*! writes track header as specified in \a properties and individual
 * tracks to the file specified in \a file. Writing individual tracks is
 * done using the append() method.
 *
 * This class implements a large write-back RAM buffer to hold the track
 * data in RAM, and only commits to file when the buffer capacity is
 * reached. This minimises the number of write() calls, which can
 * otherwise become a bottleneck on distributed or network filesystems.
 * It also helps reduce file fragmentation when multiple processes write
 * to file concurrently. The size of the write-back buffer defaults to
 * 16MB, and can be set in the config file using the
 * TrackWriterBufferSize field (in bytes).
 * */
template <typename ValueType = float> class Writer : public WriterUnbuffered<ValueType> {
public:
  using __WriterBase__<ValueType>::count;
  using __WriterBase__<ValueType>::total_count;
  using WriterUnbuffered<ValueType>::delimiter;
  using WriterUnbuffered<ValueType>::format_point;
  using WriterUnbuffered<ValueType>::weights_name;
  using WriterUnbuffered<ValueType>::write_weights;
  using vector_type = typename WriterUnbuffered<ValueType>::vector_type;

  //! create new RAM-buffered track file with specified properties
  /*! the capacity of the RAM buffer can be specified as a config file
   * option (TrackWriterBufferSize), or in the constructor by
   * specifying a value in bytes for \c default_buffer_capacity
   * (default is 16M). */
  // CONF option: TrackWriterBufferSize
  // CONF default: 16777216
  // CONF The size of the write-back buffer (in bytes) to use when
  // CONF writing track files. MRtrix will store the output tracks in a
  // CONF relatively large buffer to limit the number of write() calls,
  // CONF avoid associated issues such as file fragmentation.
  Writer(std::string_view file, const Properties &properties, size_t default_buffer_capacity = 16777216)
      : WriterUnbuffered<ValueType>(file, properties),
        buffer_capacity(File::Config::get_int("TrackWriterBufferSize", default_buffer_capacity) / sizeof(vector_type)),
        buffer(new vector_type[buffer_capacity]),
        buffer_size(0) {}

  Writer(const Writer &W) = delete;

  //! commits any remaining data to file
  ~Writer() { commit(); }

  //! append track to file
  bool operator()(const Streamline<ValueType> &tck) {
    if (buffer_size + tck.size() + 2 > buffer_capacity)
      commit();

    if (tck.size() + 1 >= buffer_capacity) {
      buffer_capacity = tck.size() + 1;
      buffer.reset(new vector_type[buffer_capacity]);
    }

    for (const auto &i : tck) {
      assert(i.allFinite());
      add_point(i);
    }
    add_point(delimiter());

    if (weights_name.size())
      weights_buffer += str(tck.weight) + ' ';

    ++count;
    ++total_count;
    return true;
  }

protected:
  size_t buffer_capacity;
  std::unique_ptr<vector_type[]> buffer;
  size_t buffer_size;
  std::string weights_buffer;

  //! add point to buffer and increment buffer_size accordingly
  void add_point(const vector_type &p) { format_point(p, buffer[buffer_size++]); }

  void commit() {
    WriterUnbuffered<ValueType>::commit(buffer.get(), buffer_size);
    buffer_size = 0;

    if (weights_name.size()) {
      write_weights(weights_buffer);
      weights_buffer.clear();
    }
  }
};

} // namespace MR::DWI::Tractography
