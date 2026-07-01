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
#include <cstdint>
#include <functional>
#include <memory>

namespace MR::DWI::Tractography::Formats {

//! \brief A format-agnostic RAM write-back buffer for tractography output.
/*! This component generalises the 16 MB RAM write-back buffer historically
 * embedded inside the ".tck" Writer (and replicated in the ".tsf"
 * ScalarWriter). It accumulates opaque binary data in a heap buffer and only
 * commits to the filesystem when the buffer fills, on an explicit flush, or on
 * destruction. Batching writes in this way minimises the number of write()
 * calls, which can otherwise become a bottleneck on distributed or network
 * filesystems, and reduces file fragmentation when multiple processes write
 * concurrently. The default capacity is 16 MB and is configurable through the
 * TrackWriterBufferSize config-file field (in bytes).
 *
 * The buffer is deliberately ignorant of the format it serves: it operates on
 * raw bytes (std::byte) rather than any vertex/scalar type, and delegates the
 * actual filesystem write to an owner-supplied callback (set_flush_callback()).
 * That callback receives the accumulated byte span plus the running
 * streamline/element counts, and is responsible for the format-specific
 * mechanics: appending the bytes to the data region and patching the relevant
 * header field (typically the streamline count, which grows as the file is
 * expanded). This is the buffer that the ".vtk", ".tt", the inter-command pipe,
 * and ".tsf" handlers are intended to reuse.
 *
 * Ownership/threading: a WriteBuffer is owned by a single writer backend and is
 * not itself thread-safe; concurrency is handled at the queue/writer level. */
class WriteBuffer {
public:
  //! \brief Running counts reported to the flush callback.
  /*! Mirrors the count / total_count fields that a tractography header must be
   * patched with each time the buffer is pushed to the filesystem. */
  struct Counts {
    uint64_t count;       //!< number of streamlines committed to the data region so far
    uint64_t total_count; //!< number of streamlines seen, including any skipped
  };

  //! \brief Signature of the owner-supplied filesystem-flush callback.
  /*! Invoked on every commit with the accumulated bytes (\a data, of length
   * \a size) and the current \a counts. The callback performs the
   * format-specific append to the data region and patches the file header. */
  using FlushCallback = std::function<void(const std::byte *data, size_t size, const Counts &counts)>;

  //! \brief Construct a buffer with the given capacity in bytes.
  /*! \a capacity_bytes is the requested capacity; it is rounded down to a whole
   * multiple of \a element_size so that no element is ever split across a
   * commit boundary. \a element_size is the size in bytes of the smallest
   * indivisible unit the owner appends (e.g. one 3-vector for ".tck", one
   * scalar for ".tsf"); it must be non-zero. */
  WriteBuffer(size_t capacity_bytes, size_t element_size);

  //! \brief commits any buffered data to the filesystem.
  ~WriteBuffer();

  WriteBuffer(const WriteBuffer &) = delete;
  WriteBuffer &operator=(const WriteBuffer &) = delete;

  //! \brief Register the callback that writes buffered bytes to the filesystem.
  /*! Must be set by the owner before any data is appended; the owner captures
   * whatever filesystem state (path, offsets, header layout) it needs. */
  void set_flush_callback(FlushCallback callback) { flush = std::move(callback); }

  //! \brief Provide the running counts to be forwarded to the flush callback.
  /*! The owner supplies a reference to its own count state so that the callback
   * always observes the up-to-date streamline counts when a commit occurs. */
  void set_counts(const Counts *counts) { counts_ptr = counts; }

  //! \brief Append \a size bytes of opaque data to the buffer.
  /*! If the incoming data would overflow the current capacity, the buffer is
   * first committed. If a single append exceeds the capacity outright, the
   * buffer is grown to accommodate it. */
  void add(const std::byte *data, size_t size);

  //! \brief Push any buffered data to the filesystem via the flush callback.
  /*! A no-op when the buffer is empty. */
  void commit();

  //! \brief Number of bytes currently held in the buffer.
  size_t size() const { return buffer_size; }

  //! \brief Current buffer capacity in bytes.
  size_t capacity() const { return buffer_capacity; }

private:
  size_t buffer_capacity;
  const size_t element_size;
  std::unique_ptr<std::byte[]> buffer;
  size_t buffer_size;
  FlushCallback flush;
  const Counts *counts_ptr;
};

} // namespace MR::DWI::Tractography::Formats
