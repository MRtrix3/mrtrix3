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

namespace MR::File {
class MMap;
}

namespace MR::DWI::Tractography::Formats::TRKUtils {

//! the fixed size in bytes of the TrackVis ".trk" header (spec: 1000 bytes)
constexpr size_t header_bytes = 1000;
//! the value the \c hdr_size field must read as; used as the byte-swap sentinel
constexpr int32_t header_size_sentinel = 1000;
//! the maximum number of scalars (dpv) / properties (dps) the header can name
constexpr size_t max_named_fields = 10;
//! the maximum length (chars) of a scalar / property name in the header
constexpr size_t name_length = 20;

//! \brief Byte-packed view of the fixed-size 1000-byte TrackVis ".trk" header.
/*! The member layout and byte offsets are taken verbatim from the TrackVis
 * format specification. The struct is packed (no padding) and built from
 * fixed-width types (wrapped in std::array, which is layout-compatible with the
 * underlying C array) so that its in-memory image matches the on-disk byte
 * layout exactly (verified by a static_assert on sizeof == 1000). Multi-byte
 * fields are stored little-endian on disk; the reader detects the opposite-endian
 * case via the \c hdr_size sentinel (must read as 1000) and byte-swaps every
 * multi-byte field on ingest. */
#pragma pack(push, 1)
struct Header {
  std::array<char, 6> id_string;                                             //!< first 5 chars are "TRACK"
  std::array<int16_t, 3> dim;                                                //!< image volume dimensions
  std::array<float, 3> voxel_size;                                           //!< voxel spacing (mm)
  std::array<float, 3> origin;                                               //!< origin (unused; always 0)
  int16_t n_scalars;                                                         //!< per-vertex scalars (→ dpv)
  std::array<std::array<char, name_length>, max_named_fields> scalar_name;   //!< scalar field names
  int16_t n_properties;                                                      //!< per-streamline props (→ dps)
  std::array<std::array<char, name_length>, max_named_fields> property_name; //!< property field names
  std::array<std::array<float, 4>, 4> vox_to_ras;                            //!< voxel→RAS affine
  std::array<char, 444> reserved;                                            //!< reserved
  std::array<char, 4> voxel_order;                                           //!< storage order of the image
  std::array<char, 4> pad2;                                                  //!< padding
  std::array<float, 6> image_orientation_patient;                            //!< DICOM image orientation
  std::array<char, 2> pad1;                                                  //!< padding
  uint8_t invert_x;                                                          //!< flags (internal use)
  uint8_t invert_y;
  uint8_t invert_z;
  uint8_t swap_xy;
  uint8_t swap_yz;
  uint8_t swap_zx;
  int32_t n_count;  //!< number of tracks (0 ⇒ unset)
  int32_t version;  //!< format version (current is 2)
  int32_t hdr_size; //!< header size; must be 1000
};
#pragma pack(pop)

static_assert(sizeof(Header) == header_bytes, "TrackVis \".trk\" header must be exactly 1000 bytes");

} // namespace MR::DWI::Tractography::Formats::TRKUtils

namespace MR::DWI::Tractography {

//! \brief Streaming reader backend for the TrackVis (".trk") format.
/*! This is the read backend for the ".trk" format handler (Formats::TRK). A
 * ".trk" file is a single binary file with a fixed 1000-byte header followed by
 * a body of variable-length streamline records. Each record is an int32 vertex
 * count, then per vertex 3 float coordinates plus \c n_scalars float scalars
 * (→ dpv), then \c n_properties float properties for the whole streamline
 * (→ dps). Per the format spec all scalars/properties are float32 on disk, so
 * they are carried as native-float sidecar fields (D7).
 *
 * Coordinates are stored in millimetres measured in the voxel space of the grid
 * the header describes (corner-referenced). They are converted to MRtrix
 * scanner-space RAS by dividing out the voxel spacing to obtain a fractional
 * voxel index, then applying the voxel→scanner transform: the supplied
 * OptionalHeader's transform when a reference grid is available, else an
 * axis-aligned default derived from the ".trk" \c vox_to_ras / \c voxel_size so
 * that a ".trk" → ".trk" round-trip is exact.
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/trk.cpp. */
template <class ValueType = float> class TRKReader : public ReaderInterface<ValueType> {
public:
  TRKReader(const std::filesystem::path &path,
            Properties &properties,
            FieldRegistry &registry,
            const OptionalHeader &grid);
  ~TRKReader() override;

  bool operator()(Streamline<ValueType> &tck) override;
  bool operator()(TractogramItem<ValueType> &item) override;

private:
  FieldRegistry &registry;

  //! whole-file memory map; the body is streamed by advancing an offset within it
  std::shared_ptr<File::MMap> mmap;
  //! byte offset of the next streamline record within the memory map
  int64_t position;
  //! one-past-the-end byte offset of the mapped data
  int64_t end;
  //! true if the file's multi-byte fields are opposite-endian to this host
  bool byte_swapped;

  //! number of per-vertex scalars (dpv) declared in the header
  size_t n_scalars;
  //! number of per-streamline properties (dps) declared in the header
  size_t n_properties;
  //! role-local dpv ordinals of the scalar fields, in on-disk column order
  std::vector<size_t> scalar_ordinals;
  //! role-local dps ordinals of the property fields, in on-disk column order
  std::vector<size_t> property_ordinals;

  //! voxel spacing (mm) recovered from the header (used to map mm ↔ voxel)
  std::array<double, 3> voxel_size;
  //! voxel→scanner transform (reference grid or ".trk"-derived default)
  transform_type voxel2scanner;

  size_t current_index;

  //! \brief read the next record's geometry into \a tck; optionally capture sidecars.
  /*! \a item is null on the vertices-only path; when non-null the per-vertex
   * scalars are gathered into its dpv payload and the per-streamline properties
   * into its dps payload. */
  bool read_record(Streamline<ValueType> &tck, TractogramItem<ValueType> *item);

  TRKReader(const TRKReader &) = delete;
};

//! \brief Streaming writer backend for the TrackVis (".trk") format.
/*! This is the write backend for the ".trk" format handler (Formats::TRK). The
 * 1000-byte header is written first (with placeholder track count), then each
 * streamline record is accumulated through the shared Formats::WriteBuffer; on
 * finalisation the \c n_count header field is patched in place with the realised
 * streamline count (the header is at a fixed offset, so unlike a pipe it can be
 * patched). Vertices are converted from MRtrix scanner-space to ".trk"
 * voxel-millimetre coordinates (scanner→voxel via the reference / default grid,
 * then scaled by the voxel spacing).
 *
 * Per-vertex dpv fields are written as \c n_scalars float scalars after each
 * vertex; per-streamline dps fields as \c n_properties float properties after
 * the streamline's vertices (D7: emitted as float, the only sidecar element type
 * ".trk" admits). Multi-column (M>1) fields expand to M consecutive named
 * columns.
 *
 * The implementation is explicitly instantiated for float and double in
 * formats/trk.cpp. */
template <class ValueType = float> class TRKWriter : public WriterInterface<ValueType> {
public:
  TRKWriter(const std::filesystem::path &path,
            const Properties &properties,
            const FieldRegistry &registry,
            const OptionalHeader &grid);
  ~TRKWriter() override;

  bool operator()(const Streamline<ValueType> &tck) override;
  bool operator()(const TractogramItem<ValueType> &item) override;

private:
  //! \brief one dpv (scalar) or dps (property) output field, with its registry ordinal.
  struct SidecarOutput {
    FieldDescriptor descriptor;
    size_t ordinal; //!< role-local ordinal into TractogramItem::dpv / ::dps
  };

  const std::filesystem::path path;
  const FieldRegistry &registry;

  //! scanner→voxel transform (inverse of the reference / default voxel→scanner)
  transform_type scanner2voxel;
  //! voxel spacing (mm) written into the header and used to map voxel → mm
  std::array<float, 3> voxel_size;
  //! voxel→RAS affine written into the header
  std::array<std::array<float, 4>, 4> vox_to_ras;
  //! grid dimensions written into the header
  std::array<int16_t, 3> dim;

  //! dpv fields, in registry order; their values become per-vertex scalars
  std::vector<SidecarOutput> scalar_fields;
  //! dps fields, in registry order; their values become per-streamline properties
  std::vector<SidecarOutput> property_fields;
  //! total scalar columns (Σ over dpv fields of M) == header n_scalars
  size_t n_scalars;
  //! total property columns (Σ over dps fields of M) == header n_properties
  size_t n_properties;

  //! accumulated streamline-record payload (the file body)
  std::filesystem::path body_tempfile;
  Formats::WriteBuffer body_buffer;
  size_t num_streamlines;

  //! assemble and write the header, then concatenate the buffered body
  void finalise();

  TRKWriter(const TRKWriter &) = delete;
};

namespace Formats {

//! \brief Format handler for the TrackVis (".trk") tractography format.
/*! The ".trk" format is a single binary file: a fixed 1000-byte header
 * (geometry, the voxel→RAS affine, and the names of any per-vertex scalars /
 * per-streamline properties) followed by variable-length streamline records.
 * Coordinates are stored in voxel-millimetres, so an external reference grid
 * (the Stage 6 OptionalHeader) is used to place streamlines in MRtrix
 * scanner-space; when none is supplied a default grid derived from the file's
 * own \c vox_to_ras / \c voxel_size is used so a ".trk" → ".trk" round-trip is
 * exact. Per-vertex scalars map to dpv and per-streamline properties to dps,
 * carried as native-float sidecar fields (D7).
 *
 * Capabilities: read+write; sequential streaming access only — the streamline
 * records are variable-length, so the dataset cannot be randomly indexed without
 * a prior full scan; rewrite-only (any change requires the whole file to be
 * rewritten). The framework therefore rejects a random-access request against
 * ".trk" with a clean error (Tractogram::require_random_access). */
class TRK : public Base {
public:
  TRK() : Base("TrackVis TRK", {IO::ReadWrite, Access::Streaming, Augment::Rewrite}) {}

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
