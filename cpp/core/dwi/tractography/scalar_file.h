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

#pragma once

#include <cstddef>
#include <map>

#include "dwi/tractography/file_base.h"
#include "dwi/tractography/formats/write_buffer.h"
#include "dwi/tractography/nonfinite.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"
#include "file/config.h"
#include "file/key_value.h"
#include "file/ofstream.h"
#include "types.h"

namespace MR::DWI::Tractography {

//! \brief non-finite tolerance of the Track Scalar File (".tsf") format.
/*! Like the ".tck" stream, ".tsf" uses NaN as the per-streamline delimiter and
 * Inf as the end-of-data barrier, so it can carry neither in real scalar data.
 * Broadcast here (the ".tsf" reader/writer are not Formats::Base handlers) so a
 * producing command can poll it, and enforced by ScalarWriter::operator(). */
inline constexpr Formats::NonFinite tsf_nonfinite_tolerance = Formats::NonFinite::Forbidden;

template <typename T = float> class ScalarReader : public ReaderBase {
public:
  using value_type = T;

  ScalarReader(const std::filesystem::path &path, Properties &properties) { open(path, "track scalars", properties); }

  bool operator()(TrackScalar<T> &tck_scalar) {
    tck_scalar.clear();

    if (!in.is_open())
      return false;
    do {
      value_type val = get_next_scalar();
      if (std::isinf(val)) {
        in.close();
        return false;
      }
      if (in.eof()) {
        in.close();
        return false;
      }

      if (std::isnan(val)) {
        tck_scalar.set_index(current_index++);
        return true;
      }
      tck_scalar.push_back(val);
    } while (in.good());

    in.close();
    return false;
  }

protected:
  using ReaderBase::current_index;
  using ReaderBase::dtype;
  using ReaderBase::in;

  value_type get_next_scalar() {
    using namespace ByteOrder;
    switch (dtype()) {
    case DataType::Float32LE: {
      float val;
      in.read((char *)&val, sizeof(val));
      return static_cast<value_type>(LE(val));
    }
    case DataType::Float32BE: {
      float val;
      in.read((char *)&val, sizeof(val));
      return static_cast<value_type>(BE(val));
    }
    case DataType::Float64LE: {
      double val;
      in.read((char *)&val, sizeof(val));
      return static_cast<value_type>(LE(val));
    }
    case DataType::Float64BE: {
      double val;
      in.read((char *)&val, sizeof(val));
      return static_cast<value_type>(BE(val));
    }
    default:
      assert(0);
      break;
    }
    return std::numeric_limits<value_type>::quiet_NaN();
  }

  ScalarReader(const ScalarReader &) = delete;
};

//! class to handle writing track scalars to file
/*! writes track scalar file header as specified in \a properties and individual
 * track scalars to the file specified in \a file. Writing individual scalars is
 * done using the append() method.
 *
 * This class implements a large write-back RAM buffer to hold the track scalar
 * data in RAM, and only commits to file when the buffer capacity is
 * reached. This minimises the number of write() calls, which can
 * otherwise become a bottleneck on distributed or network filesystems.
 * It also helps reduce file fragmentation when multiple processes write
 * to file concurrently. The size of the write-back buffer defaults to
 * 16MB, and can be set in the config file using the
 * TrackWriterBufferSize field (in bytes).
 * */
template <typename T = float> class ScalarWriter : public WriterBase<T> {
public:
  using value_type = T;
  using WriterBase<T>::count;
  using WriterBase<T>::count_offset;
  using WriterBase<T>::total_count;
  using WriterBase<T>::path;
  using WriterBase<T>::dtype;
  using WriterBase<T>::create;
  using WriterBase<T>::update_counts;
  using WriterBase<T>::verify_stream;
  using WriterBase<T>::open_success;

  ScalarWriter(const std::filesystem::path &path, const Properties &properties)
      : WriterBase<T>(path),
        buffer(File::Config::get_int("TrackWriterBufferSize", 16777216), sizeof(value_type)),
        current_offset(0) {
    File::OFStream out;
    try {
      out.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
    } catch (Exception &e) {
      throw Exception(e, "Unable to create output track scalar file");
    }

    // Do NOT set Properties timestamp here! (Must match corresponding .tck file)
    const_cast<Properties &>(properties).set_version_info();
    const_cast<Properties &>(properties).update_command_history();
    create(out, properties, "track scalars");
    open_success = true;
    current_offset = out.tellp();

    buffer.set_flush_callback(
        [this](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts & /*counts*/) {
          this->flush_scalars(data, size);
        });
  }

  ~ScalarWriter() {
    try {
      commit();
    } catch (Exception &e) {
      Exception(e, "Tractography scalar file not properly finalised").display();
    }
  }

  bool operator()(const TrackScalar<T> &tck_scalar) {
    // The ".tsf" stream uses NaN as the per-streamline delimiter and Inf as the
    //   end-of-data barrier, so a non-finite scalar would corrupt it: reject up front.
    enforce_scalars(tck_scalar, tsf_nonfinite_tolerance);
    for (typename std::vector<value_type>::const_iterator i = tck_scalar.begin(); i != tck_scalar.end(); ++i)
      add_scalar(*i);
    add_scalar(delimiter());
    ++count;
    ++total_count;
    return true;
  }

protected:
  //! format-agnostic RAM write-back buffer (Stage 2); holds formatted scalar bytes
  Formats::WriteBuffer buffer;
  int64_t current_offset;

  void add_scalar(const value_type &s) {
    value_type formatted;
    format_scalar(s, formatted);
    buffer.add(reinterpret_cast<const std::byte *>(&formatted), sizeof(value_type));
  }

  value_type delimiter() const { return std::numeric_limits<value_type>::quiet_NaN(); }

  void format_scalar(const value_type &s, value_type &destination) {
    using namespace ByteOrder;
    if (dtype.is_little_endian())
      destination = LE(s);
    else
      destination = BE(s);
  }

  //! append the buffered scalar bytes to the data region and patch the header counts
  void flush_scalars(const std::byte *data, size_t size) {
    if (size == 0 || !open_success)
      return;
    File::OFStream out(path, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
    out.seekp(current_offset, out.beg);
    out.write(reinterpret_cast<const char *>(data), size);
    current_offset = static_cast<int64_t>(out.tellp());
    verify_stream(out);
    update_counts(out);
    verify_stream(out);
  }

  void commit() { buffer.commit(); }

  ScalarWriter(const ScalarWriter &) = delete;
};

} // namespace MR::DWI::Tractography
