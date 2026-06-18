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
#include <memory>
#include <vector>

#include "datatype.h"
#include "dwi/tractography/formats/trx.h"
#include "file/mmap.h"
#include "opengl/gl_core_3_3.h"

namespace MR::GUI::MRView::Tool {

//! \brief How a fast-path vertex block delimits its constituent streamlines.
/*! Selected by each format handler when it produces a VertexBlockSource. It
 * governs both how the per-streamline boundaries (starts[] / sizes[]) are
 * established from the raw block and whether the per-LOD element buffers must
 * skip in-band delimiter slots. */
enum class BoundaryMechanism {
  //! ".tck": one all-NaN delimiter triplet follows each streamline in-band; the
  //!   boundaries are recovered by scanning the block, and the element buffers
  //!   must never index the delimiter slots (the NaN-skip property).
  NaNDelimiter,
  //! ".vtx" / TRX: an explicit array of vertex offsets delimits the streamlines;
  //!   the per-streamline vertex counts are precomputed into streamline_sizes
  //!   and the vertex block holds only real (contiguous) vertices.
  OffsetsArray,
  //! ".vtk": a LINES connectivity list delimits the streamlines; once verified
  //!   contiguous it yields the same precomputed streamline_sizes as an offsets
  //!   array and the vertex block holds only real (contiguous) vertices.
  LinesConnectivity
};

//! \brief On-disk byte order of a fast-path vertex block's coordinates.
/*! A Native block matches the host and may be memcpy'd verbatim; a BigEndian or
 * LittleEndian block whose order differs from the host is decoded coordinate by
 * coordinate in a single staging pass per chunk. */
enum class VertexByteOrder { Native, BigEndian, LittleEndian };

//! \brief A descriptor giving a fast-path loader raw access to a tractogram's
//!   contiguous vertex block plus the means to recover its streamline boundaries.
/*! Populated by a format handler when its fast-path applicability gate (datatype,
 * encoding, endianness, contiguity) passes, and consumed by
 * Tractogram::load_tracks_fast(). It owns whatever backing keeps \a block valid
 * for the duration of the load (a memory map and/or, for TRX, the TrxSource that
 * resolved each member to a byte range). When a handler cannot serve a file by
 * raw block it returns std::nullopt and the generic per-streamline loader runs
 * instead. */
struct VertexBlockSource {
  //! head of the contiguous run of on-disk vertex triplets (one 3-vector per slot)
  const std::byte *block{nullptr};
  //! total number of triplet slots in \a block. For NaNDelimiter this counts the
  //!   in-band delimiter slots and the trailing barrier is excluded; for the
  //!   offsets/lines mechanisms it equals the real-vertex count.
  int64_t num_slots{0};

  //! the on-disk vertex element datatype (Float16 or Float32); Float64 is gated
  //!   out by the handler so the GPU never sees an f64 fast path
  DataType element_datatype;
  //! the on-disk byte order of the coordinates (governs verbatim vs staged copy)
  VertexByteOrder byte_order{VertexByteOrder::Native};

  //! the streamline-boundary mechanism (selects the boundary recovery + EBO rule)
  BoundaryMechanism boundary{BoundaryMechanism::OffsetsArray};
  //! per-streamline real vertex counts, in order; populated for OffsetsArray and
  //!   LinesConnectivity (empty for NaNDelimiter, which scans the block in place)
  std::vector<GLint> streamline_sizes;

  //! the datatype to publish as properties.vertex_datatype (carries byte order)
  DataType file_datatype;

  //! backing keep-alive: the memory map (".tck"/".vtk"/".vtx"), if any
  std::shared_ptr<File::MMap> mmap;
  //! backing keep-alive: the TRX source owning the member byte ranges, if any
  std::shared_ptr<MR::DWI::Tractography::Formats::TRXUtils::TrxSource> source;
};

} // namespace MR::GUI::MRView::Tool
