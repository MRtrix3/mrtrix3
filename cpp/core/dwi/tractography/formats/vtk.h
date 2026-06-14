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
 * (DATASET POLYDATA) format, restricted to the POINTS and LINES topology fields
 * plus the optional POINT_DATA (→ dpv) and CELL_DATA (→ dps) sidecar attribute
 * sections (Stage 13). Any other dataset structure is rejected.
 *
 * On construction the ASCII keyword lines are parsed to locate the byte offsets
 * of the POINTS and LINES blocks. The LINES block is scanned immediately, both
 * to verify that every streamline is constituted by a sequentially-ordered run
 * of vertex indices (the only topology MRtrix streamlines admit) and to record
 * the per-streamline vertex count in RAM. The POINTS block, and each discovered
 * POINT_DATA / CELL_DATA attribute block, is accessed through the file
 * memory-map (binary) or parsed into RAM (ASCII); sequential reads advance an
 * independent pointer through each block, consuming the appropriate number of
 * tuples per streamline (n_vertices for a dpv block, one for a dps block).
 *
 * Each discovered sidecar attribute is registered on the supplied FieldRegistry
 * (§2.5) with its role (dpv/dps), native datatype and column count M, so the
 * per-item dps/dpv payloads can be addressed by ordinal.
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/vtk.cpp. */
template <class ValueType = float> class VTKReader : public ReaderInterface<ValueType> {
public:
  VTKReader(const std::filesystem::path &path, Properties &properties, FieldRegistry &registry);
  ~VTKReader() override;

  bool operator()(Streamline<ValueType> &tck) override;
  bool operator()(TractogramItem<ValueType> &item) override;

  //! \brief streaming state of one POINT_DATA (dpv) or CELL_DATA (dps) attribute.
  /*! Each sidecar attribute block has its own memory-mapping cursor (binary) or
   * RAM-resident value array (ASCII) plus a running tuple pointer, stepped per
   * streamline (mirroring the POINTS streaming). The native datatype and column
   * count M are recorded so values are decoded losslessly (D7). Public so the
   * dtype-generic per-tuple readers in vtk.cpp can reference it. */
  struct SidecarBlock {
    std::string name;
    FieldRole role;            //!< DPV (POINT_DATA) or DPS (CELL_DATA)
    DataType dtype;            //!< native on-disk element datatype
    size_t columns;            //!< the field's column count M
    size_t ordinal;            //!< role-local payload-vector slot (registry)
    int64_t binary_offset;     //!< byte offset of the first value (binary)
    std::vector<double> ascii; //!< all values, row-major (ASCII); empty if binary
    size_t cursor;             //!< index of the next tuple to consume
    //! \brief ASCII COLOR_SCALARS store floats 0..1; rescale to the native 0..255.
    bool ascii_color;
  };

private:
  FieldRegistry &registry;

  //! the encoding of the POINTS / LINES binary payload
  VTKUtils::Encoding encoding;

  //! memory-map over the whole file, used to read the LINES / POINTS / attribute blocks
  std::shared_ptr<File::MMap> mmap;
  //! shared POINTS accessor (ASCII RAM or binary mmap), provided by VTKUtils
  std::unique_ptr<VTKUtils::PointReader<ValueType>> points;

  //! the discovered sidecar attribute blocks (POINT_DATA → dpv, CELL_DATA → dps)
  std::vector<SidecarBlock> sidecars;

  //! per-streamline vertex counts, established by the up-front LINES scan
  std::vector<uint32_t> streamline_sizes;
  //! ordinal of the next streamline to be yielded
  size_t current_streamline;
  //! index of the next vertex to be consumed from the POINTS block
  size_t current_vertex;
  size_t current_index;

  //! consume the next streamline's values from one dps / dpv sidecar block.
  DPSValue read_dps(SidecarBlock &block);
  DPVValue read_dpv(SidecarBlock &block, size_t n_vertices);

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
 * Formats::WriteBuffer so that filesystem writes are batched. Sidecar fields
 * (Stage 13) are likewise streamed each to its OWN temporary file: every dpv
 * field is written under POINT_DATA, every dps field under CELL_DATA. Only once
 * processing is complete and every buffer has been flushed are the temporary
 * files concatenated, behind the dataset header, into the final ".vtk".
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/vtk.cpp. */
template <class ValueType = float> class VTKWriter : public WriterInterface<ValueType> {
public:
  VTKWriter(const std::filesystem::path &path, const Properties &properties, const FieldRegistry &registry);
  ~VTKWriter() override;

  bool operator()(const Streamline<ValueType> &tck) override;
  bool operator()(const TractogramItem<ValueType> &item) override;

private:
  //! \brief one sidecar field's own temporary file + write-back buffer (Stage 13).
  /*! Each unique dps/dpv field is written to its own File::create_tempfile()
   * temporary, fed through its own Formats::WriteBuffer (consistent with how the
   * POINTS / LINES temporaries already work), then concatenated behind its
   * attribute header on finalisation. The descriptor records the field's role
   * (DPV → POINT_DATA, DPS → CELL_DATA), native datatype and column count M. */
  struct SidecarOutput {
    FieldDescriptor descriptor;
    std::filesystem::path tempfile;
    std::shared_ptr<Formats::WriteBuffer> buffer;
  };

  const std::filesystem::path path;
  const FieldRegistry &registry;
  VTKUtils::Encoding encoding;

  //! the two independent temporary files holding the POINTS and LINES payloads
  std::filesystem::path points_tempfile;
  std::filesystem::path lines_tempfile;

  //! byte-oriented write-back buffers for the two temporary files (§ Stage 2)
  Formats::WriteBuffer points_buffer;
  Formats::WriteBuffer lines_buffer;

  //! one own temporary file + buffer per sidecar field (Stage 13)
  std::vector<SidecarOutput> sidecars;

  //! running tally of vertices written (== next vertex index)
  size_t num_points;
  //! running tally of streamlines written
  size_t num_lines;
  //! total number of integers in the LINES connectivity list
  size_t lines_list_size;

  //! append one streamline's connectivity record to the LINES temporary file
  void add_line(size_t first_vertex, size_t num_vertices);

  //! append one streamline's sidecar values to each field's temporary file
  void add_sidecars(const TractogramItem<ValueType> &item);

  //! emit one sidecar field's attribute header + concatenated payload to \a out
  void append_sidecar(std::ofstream &out, const SidecarOutput &field);

  //! flush the buffers and assemble the final ".vtk" from the temporary files
  void finalise();

  VTKWriter(const VTKWriter &) = delete;
};

//! \brief non-finite tolerance broadcast by the ".vtk" handler and enforced by its writer.
/*! Legacy VTK PolyData stores float POINTS with no in-band sentinel, so a NaN
 * vertex round-trips faithfully (an infinite vertex is forbidden, as for every
 * format); POINT_DATA/CELL_DATA sidecars are likewise raw float. */
inline constexpr Formats::NonFinite vtk_vertex_tolerance = Formats::NonFinite::NaNOnly;

namespace Formats {

//! \brief Format handler for the legacy VTK PolyData (".vtk") tractography format.
/*! The ".vtk" handler reads and writes the legacy VTK simple PolyData format
 * restricted to POINTS (streamline vertices) and LINES (per-streamline vertex
 * index runs), in both the ASCII and BINARY (big-endian) encodings.
 *
 * POINT_DATA (→ dpv) and CELL_DATA (→ dps) sidecar attributes are carried
 * (Stage 13): on read each is registered on the FieldRegistry and streamed; on
 * write each dps/dpv field is emitted to its own temporary file.
 *
 * Capabilities: read+write; sequential streaming access (the read backend
 * mmaps the POINTS / attribute blocks and advances a pointer; the write backend
 * streams to per-block temporary files concatenated on completion);
 * rewrite-only. */
class VTK : public Base {
public:
  VTK()
      : Base("VTK PolyData",
             {IO::ReadWrite,
              Access::Streaming,
              Augment::Rewrite,
              StepSize::Arbitrary,
              vtk_vertex_tolerance,
              NonFinite::Any,
              SidecarData::Rewrite}) {}

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
