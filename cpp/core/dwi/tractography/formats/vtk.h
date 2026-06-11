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
#include <string_view>
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

//! \brief Streaming reader backend for the legacy VTK PolyData format.
/*! This is the read backend for the ".vtk" format handler (Formats::VTK). It
 * supports both the ASCII and BINARY encodings of the legacy VTK PolyData
 * (DATASET POLYDATA) format, restricted to the POINTS and LINES data fields
 * (any other dataset is rejected with a user-interpretable error).
 *
 * On construction the ASCII keyword lines are parsed to locate the byte offsets
 * of the POINTS and LINES blocks. The LINES block is scanned immediately, both
 * to verify that every streamline is constituted by a sequentially-ordered run
 * of vertex indices (the only topology MRtrix streamlines admit) and to record
 * the per-streamline vertex count in RAM. The POINTS block is then accessed
 * through a memory-map; sequential reads advance a pointer through that map,
 * consuming the pre-established vertex count of each streamline in turn.
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/vtk.cpp. */
template <class ValueType = float> class VTKReader : public ReaderInterface<ValueType> {
public:
  VTKReader(const std::filesystem::path &path, Properties &properties);
  ~VTKReader() override;

  bool operator()(Streamline<ValueType> &tck) override;

private:
  //! the encoding of the POINTS / LINES binary payload
  VTKUtils::Encoding encoding;

  //! memory-map over the whole file, used to read the LINES / POINTS blocks
  std::shared_ptr<File::MMap> mmap;
  //! shared POINTS accessor (ASCII RAM or binary mmap), provided by VTKUtils
  std::unique_ptr<VTKUtils::PointReader<ValueType>> points;

  //! per-streamline vertex counts, established by the up-front LINES scan
  std::vector<uint32_t> streamline_sizes;
  //! ordinal of the next streamline to be yielded
  size_t current_streamline;
  //! index of the next vertex to be consumed from the POINTS block
  size_t current_vertex;
  size_t current_index;

  VTKReader(const VTKReader &) = delete;
};

//! \brief Streaming writer backend for the legacy VTK PolyData format.
/*! This is the write backend for the ".vtk" format handler (Formats::VTK). It
 * writes streamline vertices as the VTK POINTS array and the streamlines
 * themselves as the LINES array, each line being the sequential series of
 * vertex indices constituting that streamline. Both the ASCII and BINARY
 * (big-endian) encodings are supported; the choice is surfaced by the handler
 * (the "-ascii" command-line option) rather than by any individual command.
 *
 * The POINTS and LINES payloads are accumulated into two independent temporary
 * files created with File::create_tempfile(), each fed through the shared
 * Formats::WriteBuffer so that filesystem writes are batched. Only once
 * processing is complete and both buffers have been flushed are the temporary
 * files concatenated, behind the dataset header, into the final ".vtk".
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/vtk.cpp. */
template <class ValueType = float> class VTKWriter : public WriterInterface<ValueType> {
public:
  VTKWriter(const std::filesystem::path &path, const Properties &properties);
  ~VTKWriter() override;

  bool operator()(const Streamline<ValueType> &tck) override;

private:
  const std::filesystem::path path;
  VTKUtils::Encoding encoding;

  //! the two independent temporary files holding the POINTS and LINES payloads
  std::filesystem::path points_tempfile;
  std::filesystem::path lines_tempfile;

  //! byte-oriented write-back buffers for the two temporary files (§ Stage 2)
  Formats::WriteBuffer points_buffer;
  Formats::WriteBuffer lines_buffer;

  //! running tally of vertices written (== next vertex index)
  size_t num_points;
  //! running tally of streamlines written
  size_t num_lines;
  //! total number of integers in the LINES connectivity list
  size_t lines_list_size;

  //! append one streamline's connectivity record to the LINES temporary file
  void add_line(size_t first_vertex, size_t num_vertices);

  //! flush the buffers and assemble the final ".vtk" from the two temporary files
  void finalise();

  VTKWriter(const VTKWriter &) = delete;
};

namespace Formats {

//! \brief Format handler for the legacy VTK PolyData (".vtk") tractography format.
/*! The ".vtk" handler reads and writes the legacy VTK simple PolyData format
 * restricted to POINTS (streamline vertices) and LINES (per-streamline vertex
 * index runs), in both the ASCII and BINARY (big-endian) encodings.
 *
 * Capabilities: read+write; sequential streaming access (the read backend
 * mmaps the POINTS block and advances a pointer; the write backend streams to
 * two temporary files concatenated on completion); rewrite-only. */
class VTK : public Base {
public:
  VTK() : Base("VTK PolyData", {IO::ReadWrite, Access::Streaming, Augment::Rewrite}) {}

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
