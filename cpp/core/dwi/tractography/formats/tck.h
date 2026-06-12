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

#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <string_view>

#include "dwi/tractography/file_base.h"
#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/write_buffer.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"
#include "types.h"

namespace MR::DWI::Tractography {

//! A class to read streamlines data
/*! This is the read backend for the ".tck" format handler
 * (Formats::TCK). It streams Streamline objects from the binary ".tck"
 * data, NaN-vertex delimiting streamlines and an Inf-vertex marking the
 * end-of-data barrier, and itself loads any "-tck_weights_in" sidecar.
 *
 * On-disk vertex data may be stored as half- (\c Eigen::half), single- or
 * double-precision floating-point in either byte order; each is converted to
 * the in-memory \c ValueType on read (see get_next_point()). NaN/Inf
 * delimiters survive this conversion, so the streaming protocol is unaffected.
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/tck.cpp. */
template <class ValueType = float> class Reader : public ReaderBase, public ReaderInterface<ValueType> {
public:
  //! open the \c file for reading and load header into \c properties
  Reader(const std::filesystem::path &path, Properties &properties);

  //! fetch next track from file
  bool operator()(Streamline<ValueType> &tck) override;

protected:
  using ReaderBase::current_index;
  using ReaderBase::dtype;
  using ReaderBase::in;

  Eigen::Matrix<ValueType, Eigen::Dynamic, 1> weights;

  //! takes care of byte ordering issues
  Eigen::Matrix<ValueType, 3, 1> get_next_point();

  //! Check that the weights file does not contain excess entries
  void check_excess_weights();

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
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/tck.cpp. */
template <class ValueType = float>
class WriterUnbuffered : public WriterBase<ValueType>, public WriterInterface<ValueType> {
public:
  using WriterBase<ValueType>::count;
  using WriterBase<ValueType>::total_count;
  using WriterBase<ValueType>::path;
  using WriterBase<ValueType>::dtype;
  using WriterBase<ValueType>::create;
  using WriterBase<ValueType>::verify_stream;
  using WriterBase<ValueType>::update_counts;
  using WriterBase<ValueType>::open_success;

  using vector_type = Eigen::Matrix<ValueType, 3, 1>;

  //! \brief whether to auto-detect the output weights path from the CLI option.
  /*! By default a writer reads the global "-tck_weights_out" option and routes
   * the per-streamline weights to that single file. A command that writes many
   * tractograms (e.g. connectome2tck, one per edge/node) manages a distinct
   * weights path per file and must suppress that auto-detection, setting each
   * file's path explicitly via set_weights_path() instead. */
  enum class WeightsAutoDetect { Enabled, Disabled };

  //! create a new track file with the specified properties
  WriterUnbuffered(const std::filesystem::path &path,
                   const Properties &properties,
                   WeightsAutoDetect weights_autodetect = WeightsAutoDetect::Enabled);

  //! append track to file
  bool operator()(const Streamline<ValueType> &tck) override;

  //! set the path to the track weights
  void set_weights_path(const std::filesystem::path &path);

protected:
  std::filesystem::path weights_path;
  int64_t barrier_addr;

  //! indicates end of track and start of new track
  vector_type delimiter() const { return vector_type::Constant(std::numeric_limits<ValueType>::quiet_NaN()); }
  //! indicates end of data
  vector_type barrier() const { return vector_type::Constant(std::numeric_limits<ValueType>::infinity()); }

  //! perform per-point byte-swapping if required
  void format_point(const vector_type &src, vector_type &dest);

  //! write track weights data to file
  void write_weights(std::string_view contents);

  //! write track point data to file
  /*! \note \c buffer needs to be greater than \c num_points by one
   * element to add the barrier. */
  void commit(vector_type *data, size_t num_points);

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
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/tck.cpp. */
template <typename ValueType = float> class Writer : public WriterUnbuffered<ValueType> {
public:
  using WriterBase<ValueType>::count;
  using WriterBase<ValueType>::total_count;
  using WriterUnbuffered<ValueType>::delimiter;
  using WriterUnbuffered<ValueType>::format_point;
  using WriterUnbuffered<ValueType>::weights_path;
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
  Writer(const std::filesystem::path &path, const Properties &properties, size_t default_buffer_capacity = 16777216);

  Writer(const Writer &W) = delete;

  //! commits any remaining data to file
  ~Writer() override;

  //! append track to file
  bool operator()(const Streamline<ValueType> &tck) override;

protected:
  //! format-agnostic RAM write-back buffer (Stage 2); holds formatted point bytes
  Formats::WriteBuffer buffer;
  std::string weights_buffer;

  //! append one already-formatted point to the byte buffer
  void add_point(const vector_type &p) {
    vector_type formatted;
    format_point(p, formatted);
    buffer.add(reinterpret_cast<const std::byte *>(&formatted[0]), sizeof(vector_type));
  }

  //! push the buffered point bytes to the filesystem, applying the .tck barrier protocol
  void flush_points(const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &counts);

  void commit();
};

namespace Formats {

//! \brief Format handler for the in-house MRtrix3 ".tck" tractography format.
/*! The ".tck" format is a plaintext header followed by a binary stream of
 * 3-vectors (NaN-delimited streamlines, Inf end-of-data barrier). It is the
 * first concrete handler in the tractography format subsystem and the
 * template against which subsequent handlers are modelled.
 *
 * Capabilities: read+write; sequential streaming access (the binary stream is
 * consumed in order); rewrite-only for structural change, although growth by
 * appending streamlines is supported during a single writing pass.
 *
 * The read/create factories manufacture the co-located Reader / Writer
 * backends that perform the byte-level streaming I/O. */
class TCK : public Base {
public:
  TCK() : Base("MRtrix tracks", {IO::ReadWrite, Access::Streaming, Augment::Rewrite}) {}

  bool handles(const std::filesystem::path &path) const override;

protected:
  std::unique_ptr<ReaderInterface<float>> read_float(const std::filesystem::path &path,
                                                     Properties &properties,
                                                     FieldRegistry &registry,
                                                     const OptionalHeader &grid) const override;
  std::unique_ptr<ReaderInterface<double>> read_double(const std::filesystem::path &path,
                                                       Properties &properties,
                                                       FieldRegistry &registry,
                                                       const OptionalHeader &grid) const override;
  std::unique_ptr<WriterInterface<float>> create_float(const std::filesystem::path &path,
                                                       const Properties &properties,
                                                       const FieldRegistry &registry,
                                                       const OptionalHeader &grid) const override;
  std::unique_ptr<WriterInterface<double>> create_double(const std::filesystem::path &path,
                                                         const Properties &properties,
                                                         const FieldRegistry &registry,
                                                         const OptionalHeader &grid) const override;
};

} // namespace Formats

} // namespace MR::DWI::Tractography
