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
#include <optional>
#include <string>
#include <string_view>

#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/mrtrix_base.h"
#include "dwi/tractography/formats/write_buffer.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"
#include "types.h"

namespace MR::DWI::Tractography {

//! A class to read streamlines data
/*! This is the read backend for the ".tck" format handler
 * (Formats::TCK). It streams Streamline objects from the binary ".tck"
 * data, NaN-vertex delimiting streamlines and an Inf-vertex marking the
 * end-of-data barrier. Per-streamline weights are not handled here: they are
 * routed into Streamline::weight by the framework-level weight loader
 * (dwi/tractography/weights.h), independent of the format.
 *
 * On-disk vertex data may be stored as half- (\c Eigen::half), single- or
 * double-precision floating-point in either byte order; each is converted to
 * the in-memory \c ValueType on read (see get_next_point()). NaN/Inf
 * delimiters survive this conversion, so the streaming protocol is unaffected.
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/tck.cpp. */
template <class ValueType = float> class TCKReader : public ReaderBase, public ReaderInterface<ValueType> {
public:
  //! open the \c file for reading and load header into \c properties
  TCKReader(const std::filesystem::path &path, Properties &properties);

  //! fetch next track from file
  bool operator()(Streamline<ValueType> &tck) override;

protected:
  using ReaderBase::current_index;
  using ReaderBase::dtype;
  using ReaderBase::in;

  //! takes care of byte ordering issues
  Eigen::Matrix<ValueType, 3, 1> get_next_point();

  TCKReader(const TCKReader &) = delete;
};

//! class to handle writing tracks to file, with a RAM write-back buffer
/*! Writes the track header as specified in \a properties and individual tracks
 * to the file specified by \a path; each track is appended via operator().
 *
 * Track data are accumulated in a RAM write-back buffer (Formats::WriteBuffer)
 * and committed to the filesystem when the buffer fills, on finalisation, or on
 * destruction. Batching writes this way minimises the number of write() calls (a
 * bottleneck on distributed / network filesystems) and reduces fragmentation when
 * several processes write concurrently. The buffer capacity defaults to the
 * TrackWriterBufferSize config field (16 MB); a capacity of 0 instead grows the
 * buffer only as far as the longest streamline encountered, flushing each
 * streamline as it arrives — the appropriate choice for a command that holds a
 * very large number of output files open at once (e.g. connectome2tck, one per
 * edge/node), where a full per-file buffer would exhaust memory.
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/tck.cpp. */
template <typename ValueType = float>
class TCKWriter : public WriterBase<ValueType>, public WriterInterface<ValueType> {
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

  //! create a new track file with the specified properties
  // CONF option: TrackWriterBufferSize
  // CONF default: 16777216
  // CONF The size of the write-back buffer (in bytes) to use when
  // CONF writing track files. MRtrix will store the output tracks in a
  // CONF relatively large buffer to limit the number of write() calls,
  // CONF avoid associated issues such as file fragmentation.
  TCKWriter(const std::filesystem::path &path,
            const Properties &properties,
            std::optional<size_t> buffer_capacity = std::nullopt);

  TCKWriter(const TCKWriter &) = delete;

  //! commits any remaining data to file
  ~TCKWriter() override;

  //! append track to file
  bool operator()(const Streamline<ValueType> &tck) override;

  //! record a streamline seen but not exported (advances total_count only)
  void note_unexported() override { this->skip(); }

protected:
  int64_t barrier_addr;
  //! format-agnostic RAM write-back buffer (Stage 2); holds formatted point bytes
  Formats::WriteBuffer buffer;

  //! indicates end of track and start of new track
  vector_type delimiter() const { return vector_type::Constant(std::numeric_limits<ValueType>::quiet_NaN()); }
  //! indicates end of data
  vector_type barrier() const { return vector_type::Constant(std::numeric_limits<ValueType>::infinity()); }

  //! perform per-point byte-swapping if required
  void format_point(const vector_type &src, vector_type &dest);

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

//! \brief non-finite tolerance broadcast by the ".tck" handler and enforced by its writer.
/*! The ".tck" binary stream delimits streamlines with a NaN vertex and marks
 * end-of-data with an Inf vertex, so it can carry neither in real vertex data.
 * ".tck" carries no per-vertex / per-streamline sidecar data. */
inline constexpr Formats::NonFinite tck_vertex_tolerance = Formats::NonFinite::Forbidden;

namespace Formats {

//! \brief Location and on-disk type of a ".tck" file's binary vertex block.
/*! Returned by TCK::binary_layout() to let a consumer (e.g. mrview's GPU
 * fast-path loader) read the contiguous 3-vector stream directly, bypassing the
 * per-streamline streaming reader. \a data_path is the file actually holding the
 * binary block (the ".tck" itself unless the header's "file:" entry redirects
 * elsewhere), \a data_offset its byte offset, and \a datatype the on-disk vertex
 * element type and byte order verbatim from the header — including \c Float16,
 * which the streaming reader (TCKReader) supports but which the generic header
 * validator (ReaderBase::open) rejects. */
struct TCKBinaryLayout {
  std::filesystem::path data_path;
  int64_t data_offset;
  DataType datatype;
};

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
 * The read/create factories manufacture the co-located TCKReader / TCKWriter
 * backends that perform the byte-level streaming I/O. */
class TCK : public Base {
public:
  TCK()
      : Base("MRtrix tracks",
             {IO::ReadWrite,
              Access::Streaming,
              Augment::Rewrite,
              StepSize::Arbitrary,
              tck_vertex_tolerance,
              NonFinite::Forbidden}) {}

  bool handles(const std::filesystem::path &path) const override;

  //! \brief Parse only the header to locate the binary vertex block.
  /*! Returns the data file, byte offset and on-disk vertex datatype without
   * opening any vertex stream, enabling a raw-block consumer. Returns
   * std::nullopt if the header lacks the information required to locate the
   * block (e.g. no "datatype:" or "file:" entry), in which case the caller
   * should fall back to the streaming reader rather than treat it as an error.
   * Permits Float16 (unlike ReaderBase::open). */
  std::optional<TCKBinaryLayout> binary_layout(const std::filesystem::path &path) const;

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
                                                       const OptionalHeader &grid,
                                                       const WriteOptions &options) const override;
  std::unique_ptr<WriterInterface<double>> create_double(const std::filesystem::path &path,
                                                         const Properties &properties,
                                                         const FieldRegistry &registry,
                                                         const OptionalHeader &grid,
                                                         const WriteOptions &options) const override;
};

} // namespace Formats

} // namespace MR::DWI::Tractography
