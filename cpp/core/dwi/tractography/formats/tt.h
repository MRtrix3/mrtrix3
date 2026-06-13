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
#include <memory>
#include <string>
#include <vector>

#include "transform.h"
#include "types.h"

#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/write_buffer.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography {

//! \brief Streaming reader backend for the DSI Studio TinyTrack (".tt") format.
/*! This is the read backend for the ".tt" format handler (Formats::TT). A ".tt"
 * file is a gzip-compressed MATLAB Level-5 ".mat" container holding the named
 * members "dimension" (1x3 grid size), "voxel_size" (1x3 spacing mm),
 * "parameter_id" / "report" (opaque provenance strings preserved into the
 * Properties comments), and "track" (a flat, delta-encoded streamline buffer).
 *
 * On construction the whole file is gzip-inflated, the ".mat" container is
 * parsed (Formats::Mat), and the grid geometry is established: when an
 * OptionalHeader is supplied its voxel->scanner transform is used; otherwise a
 * default axis-aligned, voxel-centred grid is synthesised from the ".tt"
 * "dimension" / "voxel_size" members (geometry is then only correct up to the
 * unknown orientation/origin of the acquisition). The packed "track" buffer is
 * retained and decoded one streamline at a time on each operator() call.
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/tt.cpp. */
template <class ValueType = float> class TTReader : public ReaderInterface<ValueType> {
public:
  TTReader(const std::filesystem::path &path, Properties &properties, const OptionalHeader &grid);

  bool operator()(Streamline<ValueType> &tck) override;

private:
  //! voxel->scanner transform combining spacing with the (reference or default) affine
  transform_type voxel2scanner;
  //! the raw packed "track" buffer (§2 of the format spec)
  std::vector<std::byte> track;
  //! byte offset of the next streamline record within \c track
  size_t position;
  size_t current_index;

  TTReader(const TTReader &) = delete;
};

//! \brief Streaming writer backend for the DSI Studio TinyTrack (".tt") format.
/*! This is the write backend for the ".tt" format handler (Formats::TT). Each
 * streamline is converted from MRtrix scanner-space to fractional voxel
 * coordinates (using the OptionalHeader transform, or a default grid derived
 * from the reference image / supplied Properties when none is given), quantised
 * to 1/32-voxel integer units, and delta-encoded into the packed "track" record
 * layout. The records are accumulated through the shared Formats::WriteBuffer
 * (Stage 2). On completion the assembled "track" buffer plus the grid members
 * are serialised into a Level-5 ".mat" container and gzip-compressed to the
 * output ".tt".
 *
 * Because ".tt" stores only grid size and spacing (no full affine), the
 * scanner->voxel transform must come from an external reference grid: the
 * OptionalHeader when supplied, else an axis-aligned default. The grid size is
 * computed from the data extent when no reference is available.
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/tt.cpp. */
template <class ValueType = float> class TTWriter : public WriterInterface<ValueType> {
public:
  TTWriter(const std::filesystem::path &path, const Properties &properties, const OptionalHeader &grid);
  ~TTWriter() override;

  bool operator()(const Streamline<ValueType> &tck) override;

private:
  const std::filesystem::path path;
  //! scanner->voxel transform (inverse of the reference / default voxel->scanner)
  transform_type scanner2voxel;
  //! grid voxel counts written into the "dimension" member
  std::array<int32_t, 3> dimension;
  //! voxel spacing (mm) written into the "voxel_size" member
  std::array<float, 3> voxel_size;
  //! true once a reference grid (Header) fixed the geometry; false for the default grid
  bool grid_from_reference;
  //! opaque provenance strings recovered from the Properties (written verbatim)
  std::string parameter_id;
  std::string report;

  //! packed "track" payload accumulated through the shared write buffer
  std::filesystem::path track_tempfile;
  Formats::WriteBuffer track_buffer;
  size_t num_streamlines;

  //! assemble the ".mat" container from the accumulated track buffer and gzip-write it
  void finalise();

  TTWriter(const TTWriter &) = delete;
};

//! \brief non-finite tolerance broadcast by the ".tt" handler and enforced by its writer.
/*! TinyTrack stores vertices as quantised 1/32-voxel integers, which cannot
 * represent any non-finite coordinate. */
inline constexpr Formats::NonFinite tt_vertex_tolerance = Formats::NonFinite::Forbidden;

namespace Formats {

//! \brief Format handler for the DSI Studio TinyTrack (".tt") tractography format.
/*! The ".tt" format is a gzip-compressed MATLAB Level-5 ".mat" container whose
 * "track" member packs every streamline as a 16-byte header (vertex count and
 * absolute first vertex in 1/32-voxel integer units) followed by signed int8
 * per-axis deltas. Vertices are stored in the voxel space of a grid described
 * only by size ("dimension") and spacing ("voxel_size"); the orientation/origin
 * affine is not carried, so an external reference grid (the Stage 6
 * OptionalHeader) is required to place streamlines correctly in MRtrix
 * scanner-space. When none is supplied a default axis-aligned grid is used and
 * geometry is correct only up to that unknown affine.
 *
 * Capabilities: read+write; sequential streaming access; rewrite-only (the
 * whole gzip ".mat" must be rewritten to alter any streamline). */
class TT : public Base {
public:
  TT()
      : Base("DSI Studio TinyTrack",
             {IO::ReadWrite,
              Access::Streaming,
              Augment::Rewrite,
              StepSize::Arbitrary,
              tt_vertex_tolerance,
              NonFinite::Forbidden}) {}

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
