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
#include "dwi/tractography/tractogram_item.h"
#include "exception.h"

namespace MR::DWI::Tractography {

//! \brief A parsed tractogram-sidecar CLI reference (§2.4; Stage 11, step 4).
/*! A tractogram-sidecar argument (App::Argument::type_tractogram_data_in()/out())
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
   * (a length mismatch), true otherwise. */
  virtual bool operator()(TractogramItem<ValueType> &item) = 0;
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

} // namespace MR::DWI::Tractography
