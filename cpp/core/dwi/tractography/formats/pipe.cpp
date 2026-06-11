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

#include "dwi/tractography/formats/pipe.h"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <unistd.h>

#include "app.h"
#include "env.h"
#include "exception.h"
#include "file/config.h"
#include "file/key_value.h"
#include "file/ofstream.h"
#include "file/temp.h"
#include "mrtrix.h"
#include "signal_handler.h"

namespace MR::DWI::Tractography::Pipe {

namespace {
bool preserve_tmpfile() { return MR::get_env("MRTRIX_PRESERVE_TMPFILE", false); }
} // namespace
const bool delete_piped_files = !preserve_tmpfile();

} // namespace MR::DWI::Tractography::Pipe

namespace MR::DWI::Tractography {

namespace {

//! \brief native byte-order flag for the preamble (0 == little-endian, 1 == big-endian).
uint8_t native_endianness() {
  DataType probe = DataType::Float32;
  probe.set_byte_order_native();
  return probe.is_little_endian() ? 0 : 1;
}

//! \brief map the processing precision to its on-wire vertex-datatype code.
template <class ValueType> Pipe::VertexDataType vertex_datatype_code() {
  if constexpr (std::is_same<ValueType, double>::value)
    return Pipe::VertexDataType::Float64;
  else
    return Pipe::VertexDataType::Float32;
}

//! \brief Neutralise SIGPIPE process-wide (once) so a broken pipe yields EPIPE.
/*! MRtrix's SignalHandler installs a terminating disposition for SIGPIPE
 * (signals.h / signal_handler.cpp). For a streaming pipe writer that default is
 * fatal: a downstream command exiting early would kill the producer before it
 * could report the broken pipe. Setting SIG_IGN converts a write() to a closed
 * pipe into a recoverable errno == EPIPE, which the writer reports as a clean,
 * user-interpretable termination. The disposition is set once and deliberately
 * not restored: while the command is shutting down (exception unwinding,
 * stream-buffer flushes at exit) any further write to the dead pipe must not
 * re-raise a fatal signal. A no-op where SIGPIPE is undefined (e.g. Windows). */
void ensure_sigpipe_ignored() {
#ifdef SIGPIPE
  std::signal(SIGPIPE, SIG_IGN);
#endif
}

} // namespace

/* ************************************************************************ */
/*                          PipeReader<ValueType>                          */
/* ************************************************************************ */

template <class ValueType>
PipeReader<ValueType>::PipeReader(Properties &properties) : current_index(0), barrier_reached(false) {
  // [1] The tractogram header travels via a temp file whose path is sent on the
  //   pipe newline-terminated (Stage 9 step 1); read that path from stdin.
  std::string header_path;
  if (!std::getline(std::cin, header_path) || header_path.empty())
    throw Exception("no header filename supplied to standard input (broken pipe?)");

  if (Tractography::Pipe::delete_piped_files)
    SignalHandler::mark_file_for_deletion(header_path);

  // Parse the ".tck"-style plaintext header (offset zero) for the dataset
  //   Properties and the vertex datatype, mirroring ReaderBase::open() but
  //   without opening any on-disk data region (the data arrive on the pipe).
  properties.clear();
  dtype = DataType::Undefined;
  File::KeyValue::Reader kv(header_path, "mrtrix tracks");
  while (kv.next()) {
    const std::string key = lowercase(kv.key());
    if (key == "roi" || key == "prior_roi") {
      try {
        std::vector<std::string> V(split(kv.value(), " \t", true, 2));
        if (V.size() != 2)
          throw 1;
        properties.prior_rois.insert(std::pair<std::string, std::string>(V[0], V[1]));
      } catch (...) {
        WARN("invalid ROI specification in piped tracks header - ignored");
      }
    } else if (key == "comment") {
      properties.comments.emplace_back(std::string(kv.value()));
    } else if (key == "file") {
      // ignored on a pipe: data arrive on the stream, not at a file offset
    } else if (key == "datatype") {
      dtype = DataType::parse(kv.value());
    } else {
      add_line(properties[std::string(kv.key())], kv.value());
    }
  }
  if (dtype == DataType::Undefined)
    throw Exception("no datatype specified for piped tracks header");

  // [2] The streamline byte stream opens with a fixed preamble; validate the
  //   signature (catches a misconnected pipe) and the producer's byte order.
  Tractography::Pipe::Preamble preamble;
  read_exact(&preamble, sizeof(preamble), "stream preamble");
  if (preamble.magic != Tractography::Pipe::magic)
    throw Exception("input stream is not an MRtrix tractogram pipe stream (bad signature)");
  if (preamble.wire_version != Tractography::Pipe::wire_version)
    throw Exception("unsupported MRtrix tractogram pipe wire version (" + str(static_cast<int>(preamble.wire_version)) +
                    "); this build supports version " + str(static_cast<int>(Tractography::Pipe::wire_version)));
  if (preamble.endianness != native_endianness())
    throw Exception("cross-endian tractogram piping is not supported");

  // [3] Field-registry block: a uint32 field count, empty in this stage.
  uint32_t field_count = 0;
  read_exact(&field_count, sizeof(field_count), "field-registry block");
  if (field_count != 0)
    throw Exception("piped tractogram declares sidecar fields, which this build cannot consume");
}

template <class ValueType> bool PipeReader<ValueType>::operator()(Streamline<ValueType> &tck) {
  tck.clear();
  if (barrier_reached)
    return false;

  do {
    auto p = get_next_point();
    if (std::isinf(p[0])) {
      barrier_reached = true;
      return false;
    }
    if (std::isnan(p[0])) {
      tck.set_index(current_index++);
      tck.weight = 1.0;
      return true;
    }
    tck.push_back(p);
  } while (true);
}

template <class ValueType> Eigen::Matrix<ValueType, 3, 1> PipeReader<ValueType>::get_next_point() {
  // The stream is native-endian (the preamble enforced equal byte order), so
  //   the raw bytes are read directly into the processing precision.
  switch (dtype()) {
  case DataType::Float32LE:
  case DataType::Float32BE: {
    std::array<float, 3> p{};
    read_exact(p.data(), sizeof(p), "vertex");
    return {static_cast<ValueType>(p[0]), static_cast<ValueType>(p[1]), static_cast<ValueType>(p[2])};
  }
  case DataType::Float64LE:
  case DataType::Float64BE: {
    std::array<double, 3> p{};
    read_exact(p.data(), sizeof(p), "vertex");
    return {static_cast<ValueType>(p[0]), static_cast<ValueType>(p[1]), static_cast<ValueType>(p[2])};
  }
  default:
    throw Exception("unsupported vertex datatype in piped tractogram");
  }
}

template <class ValueType> void PipeReader<ValueType>::read_exact(void *dest, size_t size, std::string_view context) {
  std::cin.read(reinterpret_cast<char *>(dest), static_cast<std::streamsize>(size));
  if (static_cast<size_t>(std::cin.gcount()) != size)
    throw Exception("tractogram pipe closed before end-of-data marker (incomplete " + std::string(context) +
                    "; upstream command may have failed)");
}

/* ************************************************************************ */
/*                          PipeWriter<ValueType>                          */
/* ************************************************************************ */

template <class ValueType>
PipeWriter<ValueType>::PipeWriter(const Properties &properties)
    : dtype(DataType::from<ValueType>()),
      count(0),
      total_count(0),
      finalised(false),
      stream_broken(false),
      buffer(File::Config::get_int("TrackWriterBufferSize", 16777216), sizeof(vector_type)) {
  dtype.set_byte_order_native();

  if (isatty(STDOUT_FILENO))
    throw Exception("cannot create output piped tractogram: "                           //
                    "no command connected at the other end of the pipe to receive it"); //

  // Neutralise SIGPIPE so a downstream early-exit yields a reportable EPIPE
  //   rather than a fatal signal (Stage 9 step 2).
  ensure_sigpipe_ignored();

  write_header_tempfile(properties);
  write_preamble();

  buffer.set_flush_callback([this](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &) {
    this->flush_points(data, size);
  });
}

template <class ValueType> PipeWriter<ValueType>::~PipeWriter() {
  try {
    finalise();
  } catch (Exception &e) {
    Exception(e, "Piped tractogram not properly finalised").display();
  }
}

template <class ValueType> void PipeWriter<ValueType>::write_header_tempfile(const Properties &properties) {
  // [1] Serialise the ".tck"-style plaintext header to a fresh temp file with a
  //   file offset of zero, then emit its path on stdout newline-terminated.
  // Deletion is owned by the *reader* (the last consumer of the file): the
  //   writer must not mark it for deletion, else it could be removed when the
  //   writer exits before the reader has opened it.
  const std::filesystem::path header_path = File::create_tempfile(0, ".tck");

  Properties &mutable_properties = const_cast<Properties &>(properties);
  mutable_properties.set_timestamp();
  mutable_properties.set_version_info();
  mutable_properties.update_command_history();

  File::OFStream out(header_path, std::ios::out | std::ios::binary | std::ios::trunc);
  out << "mrtrix tracks\n";
  for (const auto &i : properties) {
    if (i.first != "count" && i.first != "total_count") {
      for (const auto &line : split_lines(i.second))
        out << i.first << ": " << line << "\n";
    }
  }
  for (const auto &i : properties.comments)
    out << "comment: " << i << "\n";
  for (size_t n = 0; n < properties.seeds.num_seeds(); ++n)
    out << "roi: seed " << properties.seeds[n]->get_name() << "\n";
  for (size_t n = 0; n < properties.include.size(); ++n)
    out << "roi: include " << properties.include[n].parameters() << "\n";
  for (size_t n = 0; n < properties.exclude.size(); ++n)
    out << "roi: exclude " << properties.exclude[n].parameters() << "\n";
  for (size_t n = 0; n < properties.mask.size(); ++n)
    out << "roi: mask " << properties.mask[n].parameters() << "\n";
  for (const auto &it : properties.prior_rois)
    out << "prior_roi: " << it.first << " " << it.second << "\n";
  out << "datatype: " << dtype.specifier() << "\n";
  // A pipe is not seekable, so the streamline count cannot be patched into the
  //   header after the fact; it is conveyed by the end-of-data barrier and the
  //   trailing count instead. The on-disk "file:" offset is reported as zero.
  out << "file: . 0\n";
  out << "END\n";
  if (!out.good())
    throw Exception("error writing piped tracks header file \"" + header_path.string() + "\"");
  out.close();

  std::cout << header_path.string() << "\n";
  std::cout.flush();
  if (!std::cout.good())
    throw Exception("error sending piped tracks header path (broken pipe?)");
}

template <class ValueType> void PipeWriter<ValueType>::write_preamble() {
  Tractography::Pipe::Preamble preamble{};
  preamble.magic = Tractography::Pipe::magic;
  preamble.wire_version = Tractography::Pipe::wire_version;
  preamble.endianness = native_endianness();
  preamble.vertex_datatype = static_cast<uint8_t>(vertex_datatype_code<ValueType>());
  write_stdout(reinterpret_cast<const std::byte *>(&preamble), sizeof(preamble));

  // [3] Field-registry block: empty in this stage (vertices only).
  const uint32_t field_count = 0;
  write_stdout(reinterpret_cast<const std::byte *>(&field_count), sizeof(field_count));
}

template <class ValueType> bool PipeWriter<ValueType>::operator()(const Streamline<ValueType> &tck) {
  for (const auto &p : tck) {
    assert(p.allFinite());
    add_point(p);
  }
  add_point(delimiter());
  ++count;
  ++total_count;
  return true;
}

template <class ValueType> void PipeWriter<ValueType>::add_point(const vector_type &p) {
  // The preamble enforces equal byte order, so the native bytes are streamed
  //   directly with no per-point byte-swap.
  buffer.add(reinterpret_cast<const std::byte *>(&p[0]), sizeof(vector_type));
}

template <class ValueType> void PipeWriter<ValueType>::flush_points(const std::byte *data, size_t size) {
  write_stdout(data, size);
}

template <class ValueType> void PipeWriter<ValueType>::write_stdout(const std::byte *data, size_t size) {
  // Once the downstream read end has closed, all subsequent writes are
  //   suppressed so that finalisation (barrier + count trailer) does not
  //   re-trigger the broken-pipe condition during unwinding.
  if (stream_broken)
    return;

  // Loop ::write() until the whole span is sent, tolerating short writes and
  //   EINTR; a closed read end yields EPIPE (SIGPIPE is ignored), reported as a
  //   clean user-facing termination (Stage 9 step 2).
  size_t offset = 0;
  while (offset < size) {
    const ssize_t written = ::write(STDOUT_FILENO, data + offset, size - offset);
    if (written < 0) {
      if (errno == EINTR)
        continue;
      if (errno == EPIPE) {
        stream_broken = true;
        throw Exception("downstream command closed the tractogram pipe before all streamlines were sent");
      }
      throw Exception("error writing to tractogram pipe: " + std::string(MR::C_strerror(errno)));
    }
    offset += static_cast<size_t>(written);
  }
}

template <class ValueType> void PipeWriter<ValueType>::finalise() {
  if (finalised)
    return;
  finalised = true;

  // Commit any buffered vertices, then write the Inf end-of-data barrier and a
  //   trailing uint64 streamline count after it (the authoritative count, since
  //   the offset-zero header could not be patched on a non-seekable pipe).
  buffer.commit();

  vector_type barrier_point = barrier();
  write_stdout(reinterpret_cast<const std::byte *>(&barrier_point[0]), sizeof(vector_type));

  const uint64_t trailer_count = count;
  write_stdout(reinterpret_cast<const std::byte *>(&trailer_count), sizeof(trailer_count));
}

/* ************************************************************************ */
/*               Explicit instantiation for float and double              */
/* ************************************************************************ */

template class PipeReader<float>;
template class PipeReader<double>;
template class PipeWriter<float>;
template class PipeWriter<double>;

/* ************************************************************************ */
/*                              Formats::Pipe                              */
/* ************************************************************************ */

namespace Formats {

bool Pipe::handles(const std::filesystem::path &path) const { return is_dash(path.string()); }

std::unique_ptr<ReaderInterface<float>>
Pipe::read_float(const std::filesystem::path &, Properties &properties, const OptionalHeader &) const {
  return std::make_unique<PipeReader<float>>(properties);
}

std::unique_ptr<ReaderInterface<double>>
Pipe::read_double(const std::filesystem::path &, Properties &properties, const OptionalHeader &) const {
  return std::make_unique<PipeReader<double>>(properties);
}

std::unique_ptr<WriterInterface<float>>
Pipe::create_float(const std::filesystem::path &, const Properties &properties, const OptionalHeader &) const {
  return std::make_unique<PipeWriter<float>>(properties);
}

std::unique_ptr<WriterInterface<double>>
Pipe::create_double(const std::filesystem::path &, const Properties &properties, const OptionalHeader &) const {
  return std::make_unique<PipeWriter<double>>(properties);
}

} // namespace Formats

} // namespace MR::DWI::Tractography
