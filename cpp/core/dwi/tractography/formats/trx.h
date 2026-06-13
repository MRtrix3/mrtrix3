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
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "datatype.h"
#include "transform.h"
#include "types.h"

#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/write_buffer.h"
#include "dwi/tractography/grouping.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"

namespace MR::File {
class MMap;
}

namespace MR::DWI::Tractography::Formats::TRXUtils {

//! the JSON header member of a TRX dataset (a fixed, spec-mandated basename)
constexpr std::string_view header_member = "header";
//! the spec-mandated required header keys
constexpr std::string_view key_voxel_to_rasmm = "VOXEL_TO_RASMM";
constexpr std::string_view key_dimensions = "DIMENSIONS";
constexpr std::string_view key_nb_streamlines = "NB_STREAMLINES";
constexpr std::string_view key_nb_vertices = "NB_VERTICES";

//! \brief The decomposition of a TRX member filename: `name[.M].dtype` (§ trx.md).
/*! Each TRX array file's basename is the field name, its extension is the
 * datatype, and an optional middle integer is the column count M (absent ⇒
 * M=1). E.g. "color.3.uint8" → {name:"color", columns:3, dtype:UInt8}; "fa.float16"
 * → {name:"fa", columns:1, dtype:Float16}. */
struct FilenameGrammar {
  std::string name;
  size_t columns; //!< the on-disk column count M; 1 when the middle integer is absent
  DataType dtype; //!< the native on-disk element datatype the extension denotes
};

//! \brief Parse a TRX member filename of the form `name[.M].dtype`.
/*! \returns the decomposition, or std::nullopt if the extension is not a
 * recognised TRX datatype specifier (so the file is not a TRX array). */
std::optional<FilenameGrammar> parse_filename(std::string_view filename);

//! \brief Map an MR::DataType element type to its TRX dtype extension string.
std::string extension_from_dtype(DataType dtype);

//! \brief The role a TRX member plays, inferred from its directory location.
enum class MemberRole {
  Header,    //!< the JSON "header" member
  Positions, //!< the "positions" array (vertices)
  Offsets,   //!< the "offsets" array (per-streamline start vertex)
  DPS,       //!< a "dps/" member (data per streamline)
  DPV,       //!< a "dpv/" member (data per vertex)
  Groups,    //!< a "groups/" member (Stage 17)
  DPG,       //!< a "dpg/" member (Stage 17)
  Unknown    //!< not part of a recognised TRX structure
};

//! \brief A contiguous byte range exposed for one TRX member (D5).
/*! The backing is either a memory-mapped region (a directory file, or a byte
 * range within a ZIP_STORE archive — no extraction) or, for a deflate archive,
 * an extracted temporary file's mapping. The pointer is to the head of the
 * member's bytes and remains valid for the lifetime of the owning TrxSource. */
struct MemberRange {
  const std::byte *data; //!< pointer to the head of the member's bytes
  size_t size;           //!< the member's size in bytes
};

//! \brief A parsed summary of one TRX sidecar (dps/dpv) array member.
struct SidecarSummary {
  std::string name;
  FieldRole role;    //!< DPS or DPV
  DataType dtype;    //!< native on-disk element datatype
  size_t columns;    //!< column count M
  size_t rows;       //!< rows (== NB_STREAMLINES for dps, NB_VERTICES for dpv)
  MemberRange range; //!< the member's byte range
};

//! \brief A parsed summary of one TRX "groups/<name>.uint32" index table (Stage 17).
struct GroupSummary {
  std::string name;  //!< the group name (member basename)
  DataType dtype;    //!< native on-disk element datatype (uint32 per spec; any int accepted)
  size_t count;      //!< the number of index entries present
  MemberRange range; //!< the member's byte range
};

//! \brief A parsed summary of one TRX "dpg/<group>/<field>" metadata member (Stage 17).
struct DPGSummary {
  std::string group; //!< the group the metadatum is attached to (the dpg sub-folder)
  std::string field; //!< the metadata field name (member basename)
  DataType dtype;    //!< native on-disk element datatype
  size_t columns;    //!< column count M
  MemberRange range; //!< the member's byte range
};

//! \brief The verified summary of a TRX dataset's contents (steps 2–4).
/*! Produced by examining the directory/archive structure and the JSON header
 * WITHOUT loading the bulk vertex/sidecar payloads. Carries the required header
 * geometry, the positions/offsets member ranges, and one SidecarSummary per
 * dps/dpv array, which together build the field registry (§2.5) and validate
 * that the requisite data are present. */
struct DatasetSummary {
  //! geometry from the JSON header
  transform_type voxel_to_rasmm; //!< the 4×4 VOXEL_TO_RASMM affine
  std::array<uint16_t, 3> dimensions;
  uint64_t nb_streamlines;
  uint64_t nb_vertices;

  //! the positions array
  DataType positions_dtype; //!< float16/32/64
  MemberRange positions;
  //! the offsets array
  DataType offsets_dtype; //!< uint32/64
  MemberRange offsets;
  size_t offsets_count; //!< number of offset entries actually present

  //! one summary per dps/dpv array member, in directory-sorted order
  std::vector<SidecarSummary> sidecars;

  //! one summary per "groups/" index table, in directory-sorted order (Stage 17)
  std::vector<GroupSummary> groups;
  //! one summary per "dpg/<group>/<field>" metadata member (Stage 17)
  std::vector<DPGSummary> dpg;
};

//! \brief Abstract provider of TRX members as named contiguous byte ranges (D5).
/*! Three concretions parse the same logical structure (step 2/3/4):
 *   - a filesystem directory (mmap each member file);
 *   - an uncompressed ZIP_STORE archive (mmap the whole archive, point at each
 *     member's stored byte range — no extraction);
 *   - a compressed ZIP_DEFLATE archive (extract to a temp dir, then mmap).
 * Each exposes the member names plus, on demand, a member's byte range; the
 * "header" member can be fetched alone (step 4) without inflating the rest. */
class TrxSource {
public:
  virtual ~TrxSource() = default;

  //! \brief the member names present (POSIX-style relative paths, '/'-separated).
  virtual std::vector<std::string> member_names() const = 0;

  //! \brief whether a member of the given name is present.
  virtual bool has_member(std::string_view name) const = 0;

  //! \brief the byte range of the named member; throws if absent.
  /*! For an archive source the bytes are made available (mmap'd in place for
   * ZIP_STORE, extracted/mmap'd for ZIP_DEFLATE) and remain valid for this
   * source's lifetime. */
  virtual MemberRange member(std::string_view name) = 0;

  //! \brief build and verify the dataset summary from this source (steps 2–4).
  DatasetSummary summarise();

protected:
  //! \brief parse and validate the JSON "header" member into geometry fields.
  void parse_header(DatasetSummary &summary);
};

//! \brief Open the appropriate TrxSource for a TRX path (directory or archive).
/*! Selects a directory, ZIP_STORE or ZIP_DEFLATE source by inspecting the path:
 * a directory yields the directory source; a ".trx" file is probed as a zip
 * archive and the uncompressed/compressed source is chosen accordingly. The
 * returned source owns its backing (memory map and/or extracted temp dir) for
 * its lifetime. */
std::unique_ptr<TrxSource> open_source(const std::filesystem::path &path);

//! \brief whether the named member's content represents a deflate-compressed entry.
bool path_is_trx(const std::filesystem::path &path);

//! \brief The on-disk form of an existing TRX dataset (step 11 -force policy).
enum class Backing {
  Missing,             //!< the path does not exist
  Directory,           //!< a filesystem directory of members
  UncompressedArchive, //!< a ZIP_STORE archive
  CompressedArchive    //!< a (partly) ZIP_DEFLATE archive
};

//! \brief classify the backing form of an existing TRX path (step 11).
Backing classify_backing(const std::filesystem::path &path);

//! \brief whether augmenting an existing TRX dataset with a NEW sidecar member
//!   requires force-overwrite permission (-force) (step 11).
/*! Adding a new field to an existing dataset must be intuitive with respect to
 * MRtrix's -force semantics:
 *   - DIRECTORY: a new member is simply a new file written alongside the
 *     existing ones; no existing data is touched → does NOT require -force.
 *   - UNCOMPRESSED ARCHIVE (ZIP_STORE): libzip writes a modified archive by
 *     streaming to a temporary file and atomically replacing the original on
 *     zip_close(); a crash mid-write leaves the ORIGINAL archive intact (the
 *     temp copy is discarded). Verified against the pinned libzip 1.7.3:
 *     zip_close() performs this atomic replace, so adding a member is not
 *     destructive to existing data → does NOT require -force.
 *   - COMPRESSED ARCHIVE (ZIP_DEFLATE): the archive must be re-packed (its
 *     members re-deflated) to add a member, rewriting the whole file →
 *     REQUIRES -force.
 *   - MISSING: a fresh dataset; nothing to overwrite → does NOT require -force.
 */
bool augment_requires_force(const std::filesystem::path &path);

} // namespace MR::DWI::Tractography::Formats::TRXUtils

namespace MR::DWI::Tractography {

//! \brief Streaming reader backend for the TRX tractography format.
/*! A TRX dataset is a directory (or zip archive) of flat little-endian arrays
 * plus a JSON header. The reader opens the appropriate TrxSource (directory /
 * ZIP_STORE in place, or ZIP_DEFLATE via a temp-dir extraction; D5), memory-maps
 * the member arrays, and Eigen::Maps each in its native on-disk dtype. Vertex
 * positions (float16/32/64) are converted to the processing ValueType via
 * Eigen::half (D7); offsets delimit each streamline's vertex span. Sidecar
 * dps/dpv arrays are carried in their native dtype inside the TractogramItem
 * variant (§2.2), so integer/bit fields round-trip losslessly.
 *
 * Positions are stored in world (RASMM) space, matching ".tck" scanner-space, so
 * no per-vertex grid transform is applied; the VOXEL_TO_RASMM / DIMENSIONS header
 * geometry is preserved into Properties for a faithful round-trip.
 *
 * Explicitly instantiated for float and double in formats/trx.cpp. */
template <class ValueType = float> class TRXReader : public ReaderInterface<ValueType> {
public:
  TRXReader(const std::filesystem::path &path, Properties &properties, FieldRegistry &registry);
  ~TRXReader() override;

  bool operator()(Streamline<ValueType> &tck) override;
  bool operator()(TractogramItem<ValueType> &item) override;

  //! \brief populate \a grouping from the TRX groups/ and dpg/ members (Stage 17).
  void read_grouping(Grouping &grouping) override;

private:
  FieldRegistry &registry;

  std::unique_ptr<Formats::TRXUtils::TrxSource> source;
  Formats::TRXUtils::DatasetSummary summary;

  //! per-sidecar: the registry ordinal (role-local) assigned to the field
  std::vector<size_t> sidecar_ordinals;

  size_t current_streamline; //!< ordinal of the next streamline to emit

  //! \brief the start vertex index of streamline \a i (decoding the offsets array).
  uint64_t offset_at(size_t i) const;

  //! \brief read streamline \a index into \a tck; optionally fill \a item sidecars.
  bool read_streamline(size_t index, Streamline<ValueType> &tck, TractogramItem<ValueType> *item);

  TRXReader(const TRXReader &) = delete;
};

//! \brief Streaming writer backend for the TRX tractography format.
/*! Accumulates streamline vertices and sidecar (dps/dpv) values, then on
 * finalisation lays them out as the flat little-endian TRX arrays plus the JSON
 * header, emitting either a directory or a zip archive depending on the output
 * path. Positions are written float32 (lossless for the float/double processing
 * precision); offsets uint64; each sidecar field in its registry-declared native
 * dtype, with the `name[.M].dtype` filename grammar.
 *
 * The writer reuses TRXUtils for the filename grammar, dtype↔extension mapping
 * and the JSON header construction shared with the reader. Explicitly
 * instantiated for float and double in formats/trx.cpp. */
template <class ValueType = float> class TRXWriter : public WriterInterface<ValueType> {
public:
  TRXWriter(const std::filesystem::path &path, const Properties &properties, const FieldRegistry &registry);
  ~TRXWriter() override;

  bool operator()(const Streamline<ValueType> &tck) override;
  bool operator()(const TractogramItem<ValueType> &item) override;

  //! \brief register the grouping to serialise as groups/ and dpg/ members (Stage 17).
  void write_grouping(const Grouping &grouping) override;

private:
  //! \brief one dps/dpv output field, with its registry ordinal and on-disk layout.
  struct SidecarOutput {
    FieldDescriptor descriptor;
    size_t ordinal;                 //!< role-local ordinal into dps/dpv payload vectors
    std::filesystem::path tempfile; //!< accumulating temp file for this field's bytes
    std::shared_ptr<Formats::WriteBuffer> buffer;
  };

  const std::filesystem::path path;
  const FieldRegistry &registry;

  //! grid geometry written into the JSON header
  transform_type voxel_to_rasmm;
  std::array<uint16_t, 3> dimensions;

  //! accumulating temp files / buffers for positions and offsets
  std::filesystem::path positions_tempfile;
  std::shared_ptr<Formats::WriteBuffer> positions_buffer;
  std::vector<uint64_t> offsets;

  std::vector<SidecarOutput> dps_fields;
  std::vector<SidecarOutput> dpv_fields;

  //! \brief the dataset-level grouping to emit as groups/ and dpg/ members (Stage 17).
  /*! Supplied once via write_grouping() before finalisation; serialised in
   * finalise() alongside the positions/offsets/dps/dpv members. */
  Grouping grouping;

  uint64_t num_streamlines;
  uint64_t num_vertices;

  //! \brief append one streamline's vertices and sidecar values to the buffers.
  void append(const Streamline<ValueType> &tck, const TractogramItem<ValueType> *item);

  //! \brief assemble the TRX directory or archive from the buffered arrays.
  void finalise();

  TRXWriter(const TRXWriter &) = delete;
};

//! \brief non-finite tolerance broadcast by the TRX handler and enforced by its writer.
/*! TRX stores float32 vertex coordinates in a flat array indexed by a separate
 * offsets array (no in-band sentinel), so a NaN vertex round-trips faithfully (an
 * infinite vertex is forbidden, as for every format); sidecars are native-dtype. */
inline constexpr Formats::NonFinite trx_vertex_tolerance = Formats::NonFinite::NaNOnly;

namespace Formats {

//! \brief Format handler for the TRX tractography format (D1/D5/D7).
/*! TRX is a community tractography container: a directory or (un)compressed zip
 * archive of flat little-endian arrays (positions, offsets, dps/dpv sidecars)
 * plus a JSON header (VOXEL_TO_RASMM, DIMENSIONS, NB_STREAMLINES, NB_VERTICES).
 * Implemented from scratch on the MRtrix API (File::MMap, Eigen::Map, DataType,
 * nlohmann/json, libzip; D1), with native-dtype sidecar preservation (D7) and
 * in-place memory-mapping of uncompressed members (D5).
 *
 * Capabilities: read+write; RandomAccessFixed (random access is serviceable so
 * long as the streamline count and per-streamline vertex counts are unchanged —
 * the offsets array indexes any streamline directly); append for an existing
 * directory (a new sidecar member is simply written alongside), rewrite for an
 * archive. The "-force" interaction (step 11) is resolved per backing kind. */
class TRX : public Base {
public:
  TRX()
      : Base("TRX",
             {IO::ReadWrite,
              Access::RandomAccessFixed,
              Augment::Append,
              StepSize::Arbitrary,
              trx_vertex_tolerance,
              NonFinite::Any}) {}

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
