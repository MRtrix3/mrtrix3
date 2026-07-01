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

#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/write_buffer.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram_item.h"
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
//! the wire-format version implemented by this stage (vertices + sidecar)
constexpr uint8_t wire_version = 1;

//! datatype codes carried in Preamble::vertex_datatype (one per supported vertex type)
enum class VertexDataType : uint8_t { Float32 = 0, Float64 = 1 };

//! \brief End-of-data sentinel for the length-prefixed (sidecar) stream framing.
/*! In the sidecar wire format each streamline is prefixed by a uint64 vertex
 * count; an Inf-vertex barrier (used by the vertices-only framing) would be
 * indistinguishable from a count, so end-of-data is instead signalled by this
 * reserved count value. */
constexpr uint64_t end_of_data = std::numeric_limits<uint64_t>::max();

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
 * streamline stream from stdin: a fixed preamble and a field-registry block (the
 * sidecar field descriptors are reconstructed into \a registry), then the
 * streamlines. With no sidecar fields the stream is NaN-delimited vertex runs
 * terminated by an Inf barrier (the unchanged vertices-only framing); with
 * sidecar fields each streamline is length-prefixed (Step 4 packing) and carries
 * its dps/dpv payloads, terminated by the end_of_data sentinel. A premature
 * end-of-stream (upstream command died before the barrier) is reported as a
 * clean, user-interpretable error rather than silently treated as a short
 * tractogram.
 *
 * Explicitly instantiated for float and double in formats/pipe.cpp. */
template <class ValueType = float> class PipeReader : public ReaderInterface<ValueType> {
public:
  //! \brief open the pipe; \a registry is populated from the stream header.
  PipeReader(Properties &properties, FieldRegistry &registry);

  bool operator()(Streamline<ValueType> &tck) override;
  //! \brief read the next streamline together with its dps/dpv sidecar payload.
  bool operator()(TractogramItem<ValueType> &item) override;

  //! \brief read exactly \a size sidecar bytes off the pipe (used by the
  //!   dtype-generic field readers).
  void read_field_bytes(void *dest, size_t size, std::string_view context) { read_exact(dest, size, context); }

  PipeReader(const PipeReader &) = delete;

private:
  FieldRegistry &registry;
  DataType dtype;
  uint64_t current_index;
  bool barrier_reached;

  Eigen::Matrix<ValueType, 3, 1> get_next_point();
  void read_exact(void *dest, size_t size, std::string_view context);
  //! \brief parse the field-registry block of the stream header into registry.
  void read_field_registry();
  //! \brief read one dps field's M values for the current streamline (native dtype).
  DPSValue read_dps_field(const FieldDescriptor &field);
  //! \brief read one dpv field's n_vertices x M values (native dtype).
  DPVValue read_dpv_field(const FieldDescriptor &field, size_t n_vertices);
};

//! \brief Streaming writer of piped tractography data (write backend of Formats::Pipe).
/*! Serialises the ".tck"-style plaintext header to a fresh temp file (offset
 * zero), emits that file's path on stdout newline-terminated, then streams the
 * data as native-endian raw binary through a Formats::WriteBuffer whose flush
 * callback issues one looped ::write() per committed chunk to stdout. The
 * preamble is followed by a field-registry block serialising \a registry. With
 * no sidecar fields the streamlines use the unchanged ".tck"-style framing (NaN
 * delimiter between streamlines, Inf barrier at end-of-data); with sidecar
 * fields each streamline is length-prefixed and followed by its dps/dpv payloads
 * (Step 4 packing), terminated by the end_of_data sentinel. A trailing uint64
 * streamline count follows the barrier. SIGPIPE is neutralised for the writer's
 * lifetime so a downstream early-exit yields a reportable EPIPE rather than a
 * fatal signal.
 *
 * Explicitly instantiated for float and double in formats/pipe.cpp. */
template <class ValueType = float> class PipeWriter : public WriterInterface<ValueType> {
public:
  using vector_type = Eigen::Matrix<ValueType, 3, 1>;

  //! \brief open the pipe; \a registry's fields are serialised in the header.
  PipeWriter(const Properties &properties, const FieldRegistry &registry);
  ~PipeWriter() override;

  bool operator()(const Streamline<ValueType> &tck) override;
  //! \brief append a streamline together with its dps/dpv sidecar payload.
  bool operator()(const TractogramItem<ValueType> &item) override;

  PipeWriter(const PipeWriter &) = delete;

private:
  FieldRegistry registry;
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
  //! \brief buffer one streamline's dps fields (registry order) in native dtype.
  void add_dps(const TractogramItem<ValueType> &item);
  //! \brief buffer one streamline's dpv fields (registry order) in native dtype.
  void add_dpv(const TractogramItem<ValueType> &item);
  void flush_points(const std::byte *data, size_t size);
  //! write the whole span to stdout, looping over short writes / EINTR; EPIPE -> clean error
  void write_stdout(const std::byte *data, size_t size);
  void finalise();
};

//! \brief non-finite tolerance broadcast by the pipe handler and enforced by its writer.
/*! In its default (no-sidecar) framing the pipe reuses the ".tck" wire format,
 * delimiting streamlines with a NaN vertex and marking end-of-data with an Inf
 * vertex, so it cannot carry a non-finite vertex coordinate. (Sidecar data, when
 * present, travels length-prefixed in native dtype and is unrestricted.) */
inline constexpr Formats::NonFinite pipe_vertex_tolerance = Formats::NonFinite::Forbidden;

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
  Pipe()
      : Base("piped tracks",
             {IO::ReadWrite,
              Access::Streaming,
              Augment::Rewrite,
              StepSize::Arbitrary,
              pipe_vertex_tolerance,
              NonFinite::Any,
              SidecarData::Rewrite}) {}

  bool handles(const std::filesystem::path &path) const override;

  //! \brief the pipe is a one-pass stdin/stdout stream and cannot be RAM-wrapped (Stage 15).
  bool can_ram_wrap() const override { return false; }

protected:
  std::unique_ptr<ReaderInterface<float>>
  read_float(const std::filesystem::path &path, Properties &properties, FieldRegistry &registry) const override;
  std::unique_ptr<ReaderInterface<double>>
  read_double(const std::filesystem::path &path, Properties &properties, FieldRegistry &registry) const override;
  std::unique_ptr<WriterInterface<float>> create_float(const std::filesystem::path &path,
                                                       const Properties &properties,
                                                       const FieldRegistry &registry,
                                                       const WriteOptions &options) const override;
  std::unique_ptr<WriterInterface<double>> create_double(const std::filesystem::path &path,
                                                         const Properties &properties,
                                                         const FieldRegistry &registry,
                                                         const WriteOptions &options) const override;
};

} // namespace Formats

} // namespace MR::DWI::Tractography
