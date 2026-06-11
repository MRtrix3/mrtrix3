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

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/vtk_utils.h"
#include "dwi/tractography/formats/write_buffer.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"

namespace MR::File {
class MMap;
}

namespace MR::DWI::Tractography {

//! \brief Convenience alias for the shared VTK-derived I/O utilities namespace.
namespace VTKUtils = Formats::VTKUtils;

//! \brief Streaming reader backend for the experimental ".vtx" STREAMLINES format.
/*! This is the read backend for the ".vtx" format handler (Formats::VTX). It
 * supports both the ASCII and BINARY encodings of the experimental
 * VTK-derived STREAMLINES (DATASET STREAMLINES) format, restricted to the
 * POINTS and OFFSETS data fields (any other dataset is rejected with a
 * user-interpretable error).
 *
 * Unlike the legacy ".vtk" reader (which scans the whole LINES block up-front
 * to establish each streamline's vertex count), the ".vtx" reader determines
 * the per-streamline vertex count DURING streaming: it establishes a
 * memory-map over the OFFSETS block (or parses the ASCII offsets into RAM),
 * incrementally reads one offset at a time, and computes each streamline's
 * length as the difference to the prior offset. OFFSETS store the END vertex
 * index of each streamline (offsetEnd[j]); streamline j spans points
 * offsetEnd[j-1]+1 .. offsetEnd[j], with offsetEnd[-1] = -1.
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/vtx.cpp. */
template <class ValueType = float> class VTXReader : public ReaderInterface<ValueType> {
public:
  VTXReader(const std::filesystem::path &path, Properties &properties);
  ~VTXReader() override;

  bool operator()(Streamline<ValueType> &tck) override;

private:
  //! the element datatype of the OFFSETS index data
  enum class OffsetType { Int32, Int64 };

  //! the encoding of the POINTS / OFFSETS payload
  VTKUtils::Encoding encoding;
  OffsetType offset_type;

  //! memory-map over the whole file, used to read POINTS / OFFSETS in binary mode
  std::shared_ptr<File::MMap> mmap;
  //! shared POINTS accessor (ASCII RAM or binary mmap), provided by VTKUtils
  std::unique_ptr<VTKUtils::PointReader<ValueType>> points;

  //! number of streamlines (== number of OFFSETS entries)
  size_t num_streamlines;
  //! byte offset of the first OFFSETS entry (binary mode)
  int64_t offsets_offset;
  //! parsed OFFSETS for the ASCII encoding
  std::vector<int64_t> ascii_offsets;

  //! ordinal of the next streamline to be yielded
  size_t current_streamline;
  //! index of the next vertex to be consumed from the POINTS block
  size_t current_vertex;
  //! end-vertex index of the previously-yielded streamline (offsetEnd[j-1])
  int64_t previous_offset_end;
  size_t current_index;

  //! incrementally read the OFFSETS entry of streamline \a j
  int64_t get_offset_end(size_t j) const;

  VTXReader(const VTXReader &) = delete;
};

//! \brief Streaming writer backend for the experimental ".vtx" STREAMLINES format.
/*! This is the write backend for the ".vtx" format handler (Formats::VTX). It
 * writes streamline vertices as the POINTS array and the per-streamline END
 * vertex indices as the OFFSETS array (offsetEnd[j], the index of the final
 * point belonging to streamline j). Both the ASCII and BINARY (big-endian)
 * encodings are supported; the choice is surfaced by the handler (the "-ascii"
 * command-line option) rather than by any individual command.
 *
 * The POINTS and OFFSETS payloads are accumulated into two independent
 * temporary files created with File::create_tempfile(), each fed through the
 * shared Formats::WriteBuffer so that filesystem writes are batched. Only once
 * processing is complete and both buffers have been flushed are the temporary
 * files concatenated, behind the dataset header, into the final ".vtx".
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/vtx.cpp. */
template <class ValueType = float> class VTXWriter : public WriterInterface<ValueType> {
public:
  VTXWriter(const std::filesystem::path &path, const Properties &properties);
  ~VTXWriter() override;

  bool operator()(const Streamline<ValueType> &tck) override;

private:
  const std::filesystem::path path;
  VTKUtils::Encoding encoding;

  //! the two independent temporary files holding the POINTS and OFFSETS payloads
  std::filesystem::path points_tempfile;
  std::filesystem::path offsets_tempfile;

  //! byte-oriented write-back buffers for the two temporary files (§ Stage 2)
  Formats::WriteBuffer points_buffer;
  Formats::WriteBuffer offsets_buffer;

  //! running tally of vertices written (== next vertex index)
  size_t num_points;
  //! running tally of streamlines written
  size_t num_streamlines;

  //! append one streamline's END vertex index to the OFFSETS temporary file
  void add_offset(int64_t offset_end);

  //! flush the buffers and assemble the final ".vtx" from the two temporary files
  void finalise();

  VTXWriter(const VTXWriter &) = delete;
};

namespace Formats {

//! \brief Format handler for the experimental ".vtx" STREAMLINES tractography format.
/*! The ".vtx" handler reads and writes the experimental VTK-derived
 * STREAMLINES format (POINTS = streamline vertices; OFFSETS = per-streamline
 * END vertex index), in both the ASCII and BINARY (big-endian) encodings.
 * Per the spec, the order of POINTS matches the order of vertices within each
 * streamline and the order of the streamlines themselves, so no per-vertex
 * connectivity list is stored.
 *
 * Capabilities: read+write; sequential streaming access (the read backend
 * mmaps the POINTS / OFFSETS blocks and advances a pointer, computing each
 * streamline's vertex count during streaming); rewrite-only. */
class VTX : public Base {
public:
  VTX() : Base("VTK STREAMLINES", {IO::ReadWrite, Access::Streaming, Augment::Rewrite}) {}

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
