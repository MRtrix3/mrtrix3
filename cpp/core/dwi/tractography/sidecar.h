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

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram_item.h"
#include "exception.h"
#include "mrtrix.h"
#include "types.h"

namespace MR::DWI::Tractography {

//! \brief A parsed tractogram-sidecar CLI reference (§2.4; Stage 11, step 4).
/*! A tractogram-sidecar argument (App::Argument::type_tractogram_sidecar_in()/out())
 * is one of:
 *   - a BARE PATH: \c name is std::nullopt and \c dataset is the standalone
 *     sidecar file (per-streamline text/.csv/.npy, or per-vertex .tsf); or
 *   - a QUALIFIED "DATASET::NAME" reference: \c dataset is a tractography
 *     dataset path and \c name is the basename / string name of a sidecar field
 *     carried within it.
 * Parsing splits on the LAST "::" so that a Windows drive-letter path ("C:\\...",
 * a single colon) is never mistaken for a qualified reference. */
struct SidecarReference {
  std::filesystem::path dataset;   //!< the filesystem (DATASET) component
  std::optional<std::string> name; //!< the field NAME for a qualified reference; std::nullopt for a bare path
  //! \brief whether this reference is the qualified "DATASET::NAME" form.
  bool is_qualified() const { return name.has_value(); }
};

//! \brief parse a tractogram-sidecar CLI token into its components (§2.4).
SidecarReference parse_sidecar_reference(std::string_view arg);

//! \brief the field name a standalone (bare-path) sidecar reference would register.
/*! For a bare path this is the file basename (stem); for a qualified
 * "DATASET::NAME" reference it is the explicit NAME. Used by the Tractogram to
 * de-duplicate an external sidecar against a same-named field already carried
 * internally by the dataset (Stage 16, step 9), so the field is not loaded
 * twice. */
std::string sidecar_field_name(const SidecarReference &reference);

//! \brief the human-readable name of a sidecar field role, for diagnostic messages.
inline std::string sidecar_role_word(const FieldRole role) {
  switch (role) {
  case FieldRole::DPV:
    return "per-vertex (dpv)";
  case FieldRole::DPG:
    return "per-group (dpg)";
  default:
    return "per-streamline (dps)";
  }
}

//! \brief Streaming injector of standalone per-streamline / per-vertex sidecar
//!   data into the read pipeline (§2.5; Stage 11, step 5).
/*! Registered with a read Tractogram for a standalone-path input reference. The
 * loader registers exactly one field (basename = field name) in the
 * Tractogram's field registry on construction, then on each read fills that
 * field's value at its assigned ordinal in the streaming TractogramItem:
 *   - a per-streamline text/.csv file is loaded in full into RAM via
 *     File::Matrix::load_matrix() and one row is yielded per streamline;
 *   - a per-streamline .npy file is parsed (header) and its data region
 *     memory-mapped, an Eigen::Map providing one row per streamline;
 *   - a per-vertex .tsf file is streamed one streamline's scalars at a time.
 * Values are stored in float (the processing precision) regardless of the
 * on-disk element type. The qualified "DATASET::NAME" form is NOT YET
 * implemented and is rejected at construction. */
template <class ValueType> class SidecarLoader {
public:
  virtual ~SidecarLoader() = default;

  //! \brief populate this loader's field in \a item for the next streamline.
  /*! \returns false if the sidecar source is exhausted before the tractogram
   * (a length mismatch), true otherwise. A source running short is reported
   * exactly once, as a warning or — under set_strict() — as an error; it is
   * never silent, since the consuming Tractogram responds by truncating the
   * streamline stream at that point. */
  bool operator()(TractogramItem<ValueType> &item) {
    if (!load(item)) {
      report_short();
      return false;
    }
    ++consumed_;
    return true;
  }

  //! \brief report any entries the source holds beyond those it yielded.
  /*! Invoked once at end-of-stream by the owning Tractogram, for every
   * registered loader (in contrast to report_short(), which can only ever fire
   * for the one loader that ran dry first). Warns, or throws under
   * set_strict(). */
  void check_excess() {
    const std::optional<size_t> total = total_entries();
    std::string message;
    if (total.has_value()) {
      if (*total <= consumed_)
        return;
      message = describe() + " contains more entries (" + str(*total) + ") than the number of streamlines read (" +
                str(consumed_) + ")";
    } else {
      if (!probe_excess())
        return;
      message = describe() + " contains more entries than the " + str(consumed_) + " streamlines read";
    }
    if (strict_)
      throw Exception(message);
    WARN(message);
  }

  //! \brief check the source length against \a streamline_count up front, and throw on a mismatch.
  /*! Only meaningful where the source declares its length in advance
   * (total_entries()); a streamed source such as a ".tsf" is a no-op here and
   * relies on the end-of-stream checks instead. Used by a caller that demands
   * an exact correspondence (tckconvert's "-insert"), so the mismatch is
   * diagnosed before any output is created. */
  void validate_length(const size_t streamline_count) const {
    const std::optional<size_t> total = total_entries();
    if (!total.has_value() || *total == streamline_count)
      return;
    throw Exception(describe() + " contains " + str(*total) + " entries," + " but the tractogram contains " +
                    str(streamline_count) + " streamlines");
  }

  //! \brief promote a length mismatch from a warning to an error.
  void set_strict(const bool value) { strict_ = value; }

protected:
  SidecarLoader(const FieldRole role, const std::string_view name, const std::filesystem::path &path)
      : role_(role), name_(name), path_(path), consumed_(0), strict_(false), warned_short_(false) {}

  //! \brief yield this loader's field for the next streamline into \a item.
  /*! \returns false once the source is exhausted. Implementations report only
   * exhaustion; the diagnostics are the base class's responsibility. */
  virtual bool load(TractogramItem<ValueType> &item) = 0;

  //! \brief the total number of entries the source holds, where known in advance.
  /*! std::nullopt for a source that is streamed rather than resident (".tsf"),
   * whose length cannot be established without reading it. */
  virtual std::optional<size_t> total_entries() const { return std::nullopt; }

  //! \brief whether a source of unknown length holds at least one unconsumed entry.
  /*! Only called when total_entries() is std::nullopt, and only at
   * end-of-stream. */
  virtual bool probe_excess() { return false; }

private:
  FieldRole role_;
  std::string name_;
  std::filesystem::path path_;
  size_t consumed_; //!< entries yielded into the streamline stream so far
  bool strict_;
  bool warned_short_;

  //! \brief identify this loader (role, field name, source file) in a message.
  std::string describe() const {
    return sidecar_role_word(role_) + " sidecar field \"" + name_ + "\" from file \"" + path_.string() + "\"";
  }

  void report_short() {
    if (warned_short_)
      return;
    warned_short_ = true;
    const std::string message =
        describe() + " provides fewer entries (" + str(consumed_) + ") than there are streamlines in the tractogram";
    if (strict_)
      throw Exception(message);
    WARN(message + "; ceasing reading of streamline data");
  }
};

//! \brief Streaming exporter of processed per-streamline / per-vertex data to a
//!   standalone sidecar file (§2.7; Stage 11, step 6).
/*! Registered with a write Tractogram for a standalone-path output reference.
 * On each write it extracts this exporter's field from the streaming
 * TractogramItem; on destruction (or explicit finalise()) it commits:
 *   - per-streamline data accumulated during processing in an Eigen::Array<>
 *     (grown via conservativeResizeLike() on the std::vector<> expansion
 *     schedule) is written to a numerical text file or .npy via
 *     File::Matrix::save_matrix();
 *   - per-vertex data is written to a .tsf file as streamlines arrive.
 * The qualified "DATASET::NAME" form is NOT YET implemented and is rejected at
 * construction. */
template <class ValueType> class SidecarExporter {
public:
  virtual ~SidecarExporter() = default;
  //! \brief extract this exporter's field from \a item for the current streamline.
  virtual bool operator()(const TractogramItem<ValueType> &item) = 0;
  //! \brief commit any buffered per-streamline data to the filesystem.
  /*! Idempotent; also invoked by the destructor. Per-vertex (.tsf) exporters
   * write incrementally and treat this as a no-op flush. */
  virtual void finalise() = 0;
};

//! \brief construct the input sidecar loader for a standalone reference (step 5).
/*! \a reference must be a bare path (is_qualified()==false); the qualified form
 * throws the documented "not yet implemented" error. \a registry is the read
 * Tractogram's field registry, into which the loader registers its field. The
 * returned loader yields one value per streamline. */
template <class ValueType>
std::unique_ptr<SidecarLoader<ValueType>>
make_sidecar_loader(const SidecarReference &reference, Properties &properties, FieldRegistry &registry);

//! \brief construct the output sidecar exporter for a standalone reference (step 6).
/*! \a reference must be a bare path (is_qualified()==false); the qualified form
 * throws the documented "not yet implemented" error. \a is_random_access is the
 * owning Tractogram's access model: a per-vertex (.tsf) export is rejected when
 * true, because the .tsf format precludes random access (step 7). */
template <class ValueType>
std::unique_ptr<SidecarExporter<ValueType>>
make_sidecar_exporter(const SidecarReference &reference, const Properties &properties, bool is_random_access);

//! \brief construct an input sidecar loader for an explicitly named field (tckconvert -insert).
/*! Like make_sidecar_loader for a bare path, but the loaded field is registered
 * under the caller-supplied \a name and \a role rather than being inferred from the
 * file. The file kind must match the role: a per-vertex (DPV) field is loaded from
 * a ".tsf"; a per-streamline (DPS) field from a numerical text/".csv"/".npy" file.
 * A mismatch (e.g. DPV from a non-".tsf" file) throws. */
template <class ValueType>
std::unique_ptr<SidecarLoader<ValueType>> make_named_sidecar_loader(FieldRole role,
                                                                    std::string_view name,
                                                                    const std::filesystem::path &path,
                                                                    Properties &properties,
                                                                    FieldRegistry &registry);

//! \brief construct a standalone sidecar exporter bound to a payload ordinal (tckconvert -extract).
/*! Writes the field at \a ordinal (within the \a role payload vector) of each item
 * to the standalone file \a path: a per-streamline numerical matrix when \a role is
 * DPS, a per-vertex ".tsf" when \a role is DPV. \a path must carry the ".tsf" suffix
 * iff \a role is DPV; a mismatch throws. \a properties seeds the exporter (the
 * streamline count, and — for ".tsf" — the header written, including the shared
 * "timestamp"). */
template <class ValueType>
std::unique_ptr<SidecarExporter<ValueType>> make_named_sidecar_exporter(FieldRole role,
                                                                        size_t ordinal,
                                                                        const std::filesystem::path &path,
                                                                        const Properties &properties);

// ---------------------------------------------------------------------------
//  Streamline-weight I/O (the privileged Streamline::weight route)
// ---------------------------------------------------------------------------
/* The streamline weight is NOT a generic sidecar field: it is the privileged
 * Streamline::weight (the single source of truth; tractogram_item.h). The two
 * classes below route an EXTERNAL scalar file to / from that member, in contrast
 * to the SidecarLoader/SidecarExporter above which target the generic dps/dpv
 * payloads. The other explicit weight route — a NAMED field within a tractogram
 * dataset ("DATASET::NAME") — is handled inside the Tractogram itself, by
 * extracting the already-read dps field into / injecting Streamline::weight back
 * out at the designated ordinal, so it needs no separate class here. */

//! \brief Streaming injector of an external streamline-weight file into the
//!   privileged Streamline::weight.
/*! Constructed for a bare-path "-tck_weights_in" reference. The whole weight
 * vector is loaded into RAM on construction (one scalar per streamline; text/.csv
 * via File::Matrix::load_vector). On each streamline it sets streamline.weight at
 * the streamline's ordinal index; a file shorter than the tractogram truncates
 * the stream (as the legacy ".tck" reader did), a longer one warns at
 * end-of-stream (check_excess). */
template <class ValueType> class ExternalWeightLoader {
public:
  explicit ExternalWeightLoader(const std::filesystem::path &path);
  //! \brief assign streamline.weight for the streamline at its ordinal index.
  /*! \returns false if the weight file is exhausted before this streamline (fewer
   * weights than streamlines), signalling end-of-stream. */
  bool operator()(Streamline<ValueType> &streamline);
  //! \brief warn if the file carried more weights than \a streamline_count.
  void check_excess(size_t streamline_count) const;

private:
  Eigen::Matrix<ValueType, Eigen::Dynamic, 1> weights;
  std::filesystem::path source;
  bool warned_short;
};

//! \brief Streaming exporter of the privileged Streamline::weight to a standalone
//!   scalar file.
/*! Constructed for a bare-path "-tck_weights_out" reference. Accumulates one weight
 * per WRITTEN streamline, in write order (the legacy ".tck" convention: the file
 * carries one entry per exported streamline, not per streamline seen), and writes
 * the vector on finalise() / destruction. Works for any output tractography format,
 * since the weights file is independent of it. */
template <class ValueType> class ExternalWeightExporter {
public:
  ExternalWeightExporter(const std::filesystem::path &path, size_t initial_streamlines);
  ~ExternalWeightExporter();
  //! \brief append streamline.weight for the streamline just written.
  void operator()(const Streamline<ValueType> &streamline);
  //! \brief write the accumulated weights to the filesystem (idempotent).
  void finalise();

private:
  std::filesystem::path path;
  Eigen::Array<ValueType, Eigen::Dynamic, 1> data;
  size_t rows;
  bool finalised;
  void grow_to(size_t need_rows);
};

} // namespace MR::DWI::Tractography
