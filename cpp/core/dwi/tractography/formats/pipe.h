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

#include <array>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>

#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/write_buffer.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"
#include "types.h"

namespace MR::DWI::Tractography::Pipe {

//! \brief Fixed-size preamble that opens the streamline byte stream (§2.4 of pipe_design.md).
/*! Sent immediately after the temp-file header path line, this lets a
 * misconnected or truncated pipe fail fast with a clear error, declares the
 * producer's native byte order (cross-endian piping is rejected in this stage),
 * and records the vertex element datatype so the consumer knows the stride
 * without re-deriving it. The wire-format version reserves room for the Stage 10
 * sidecar registry and Stage 17 group trailers without a breaking change. */
struct Preamble {
  std::array<char, 8> magic;       //!< fixed signature identifying an MRtrix tractogram pipe stream
  uint8_t wire_version;            //!< wire-format version (starts at 1)
  uint8_t endianness;              //!< 0 == little-endian producer, 1 == big-endian producer
  uint8_t vertex_datatype;         //!< DataType specifier code for the vertex element type
  std::array<uint8_t, 5> reserved; //!< reserved (zero) padding to an 8-byte boundary
};
static_assert(sizeof(Preamble) == 16, "pipe stream preamble must be 16 bytes");

//! the magic signature (the leading bytes of every pipe stream)
constexpr std::array<char, 8> magic{'M', 'R', 't', 'r', 'i', 'x', 'T', 'P'};
//! the wire-format version implemented by this stage (vertices only)
constexpr uint8_t wire_version = 1;

//! datatype codes carried in Preamble::vertex_datatype (one per supported vertex type)
enum class VertexDataType : uint8_t { Float32 = 0, Float64 = 1 };

//! \brief whether the temp header file exchanged over the pipe is deleted on exit.
/*! Mirrors the image pipe handler's MRTRIX_PRESERVE_TMPFILE behaviour: the
 * header temp file is normally marked for deletion at command completion, but
 * may be preserved (e.g. for the Python API chaining piped commands). */
extern const bool delete_piped_files;

} // namespace MR::DWI::Tractography::Pipe

namespace MR::DWI::Tractography {

//! \brief Streaming reader of piped tractography data (read backend of Formats::Pipe).
/*! Reads the newline-terminated temp-file header path from stdin, parses that
 * ".tck"-style plaintext header into \a properties, then consumes the raw-binary
 * streamline stream from stdin: a fixed preamble, an (empty, in this stage)
 * field-registry block, and NaN-delimited vertex runs terminated by an Inf
 * end-of-data barrier. A premature end-of-stream (upstream command died before
 * the barrier) is reported as a clean, user-interpretable error rather than
 * silently treated as a short tractogram.
 *
 * Explicitly instantiated for float and double in formats/pipe.cpp. */
template <class ValueType = float> class PipeReader : public ReaderInterface<ValueType> {
public:
  PipeReader(Properties &properties);

  bool operator()(Streamline<ValueType> &tck) override;

  PipeReader(const PipeReader &) = delete;

private:
  DataType dtype;
  uint64_t current_index;
  bool barrier_reached;

  Eigen::Matrix<ValueType, 3, 1> get_next_point();
  void read_exact(void *dest, size_t size, std::string_view context);
};

//! \brief Streaming writer of piped tractography data (write backend of Formats::Pipe).
/*! Serialises the ".tck"-style plaintext header to a fresh temp file (offset
 * zero), emits that file's path on stdout newline-terminated, then streams the
 * vertex data as native-endian raw binary through a Formats::WriteBuffer whose
 * flush callback issues one looped ::write() per committed chunk to stdout. The
 * stream is framed exactly as the ".tck" binary convention (NaN delimiter
 * between streamlines, Inf barrier at end-of-data), with a trailing uint64
 * streamline-count after the barrier. SIGPIPE is neutralised for the writer's
 * lifetime so a downstream early-exit yields a reportable EPIPE rather than a
 * fatal signal.
 *
 * Explicitly instantiated for float and double in formats/pipe.cpp. */
template <class ValueType = float> class PipeWriter : public WriterInterface<ValueType> {
public:
  using vector_type = Eigen::Matrix<ValueType, 3, 1>;

  PipeWriter(const Properties &properties);
  ~PipeWriter() override;

  bool operator()(const Streamline<ValueType> &tck) override;

  PipeWriter(const PipeWriter &) = delete;

private:
  DataType dtype;
  uint64_t count;
  uint64_t total_count;
  bool finalised;
  bool stream_broken;
  Formats::WriteBuffer buffer;

  //! indicates end of one streamline and start of the next
  vector_type delimiter() const { return vector_type::Constant(std::numeric_limits<ValueType>::quiet_NaN()); }
  //! indicates end of all data
  vector_type barrier() const { return vector_type::Constant(std::numeric_limits<ValueType>::infinity()); }

  void write_header_tempfile(const Properties &properties);
  void write_preamble();
  void add_point(const vector_type &p);
  void flush_points(const std::byte *data, size_t size);
  //! write the whole span to stdout, looping over short writes / EINTR; EPIPE -> clean error
  void write_stdout(const std::byte *data, size_t size);
  void finalise();
};

namespace Formats {

//! \brief Format handler for inter-command Unix-pipe tractography streaming.
/*! Selected when the dataset path is the dash token "-" (is_dash()), exactly as
 * the image pipe handler keys off "-". On write it streams the tractogram
 * through stdout as native-endian raw binary; on read it reconstructs it from
 * stdin (see PipeReader / PipeWriter). The tractogram header travels via a
 * temp file whose path is exchanged newline-terminated over the pipe, while the
 * streamline vertices travel as the raw-binary stream.
 *
 * Capabilities: read+write; streaming access only (a pipe is sequential by
 * nature, so a random-access request against "-" is rejected with a clean
 * error rather than silently buffering the whole tractogram); rewrite-only. */
class Pipe : public Base {
public:
  Pipe() : Base("piped tracks", {IO::ReadWrite, Access::Streaming, Augment::Rewrite}) {}

  bool handles(const std::filesystem::path &path) const override;

protected:
  std::unique_ptr<ReaderInterface<float>>
  read_float(const std::filesystem::path &path, Properties &properties, const OptionalHeader &grid) const override;
  std::unique_ptr<ReaderInterface<double>>
  read_double(const std::filesystem::path &path, Properties &properties, const OptionalHeader &grid) const override;
  std::unique_ptr<WriterInterface<float>> create_float(const std::filesystem::path &path,
                                                       const Properties &properties,
                                                       const OptionalHeader &grid) const override;
  std::unique_ptr<WriterInterface<double>> create_double(const std::filesystem::path &path,
                                                         const Properties &properties,
                                                         const OptionalHeader &grid) const override;
};

} // namespace Formats

} // namespace MR::DWI::Tractography
