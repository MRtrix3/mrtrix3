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
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "types.h"

#include "dwi/tractography/sidecar.h"

namespace MR::DWI::Tractography::Formats {
class Base;
}

namespace MR::DWI::Tractography {

//! \brief How a recognised tractography-sidecar destination is to be written (§2.7).
/*! The three ways a tractography dataset named by a sidecar reference can receive a
 * new field, decided from its on-disk state and the handler's capabilities. A bare
 * non-tractography path (handler == nullptr) is handled by the caller as a
 * standalone file and is not represented here. */
enum class SidecarDestination {
  CreateDataset, //!< the dataset does not exist: create it from the input tractogram
  AppendInPlace, //!< the dataset exists and can take a new sidecar member in place
  RewriteDataset //!< the dataset exists but must be rewritten to add a field
};

//! \brief classify how a recognised tractography destination is to be written.
/*! \a handler is the (non-null) handler for \a dataset. \pre the format can carry
 * sidecar data (capabilities.sidecar_data != Unsupported); the caller checks this
 * so it can phrase a context-specific error. */
SidecarDestination classify_sidecar_destination(const Formats::Base &handler, const std::filesystem::path &dataset);

//! \brief a non-existent scratch path beside \a dest sharing its extension.
/*! Used as the scratch destination for the rewrite case so the final move onto
 * \a dest is a same-filesystem (atomic) rename, and so the recognised format
 * handler matches the destination's (selection is by extension). The path is not
 * created here; the writer creates it. */
std::filesystem::path make_sibling_path(const std::filesystem::path &dest);

//! \brief Export a computed per-streamline value column to the destination
//!   implied by a tractogram-sidecar reference (§2.7).
/*! A single entry point shared by any command that produces one value per
 * streamline (e.g. SIFT2 weights). The destination form is selected from
 * \a reference and the capabilities of the recognised tractography format:
 *   - the reference is a BARE PATH that no tractography handler recognises:
 *     \a values are written as a standalone numerical vector (text/.csv/.npy);
 *   - the destination is a tractography format that cannot carry per-streamline
 *     sidecar data: an error is raised;
 *   - the destination DOES NOT EXIST: a copy of the input tractogram
 *     (\a input_path) is written to it, carrying a new per-streamline (dps)
 *     field holding \a values;
 *   - the destination EXISTS and the format can append a sidecar member in place
 *     (a TRX directory or uncompressed archive): the field is written directly as
 *     a new member, with no rewrite of the streamline data; "-force" is required
 *     only if a field of this name already exists;
 *   - the destination EXISTS and the format must be rewritten to add a field
 *     (".trk", a compressed TRX archive): "-force" is required, and the augmented
 *     dataset is written to a sibling temporary file then atomically moved over
 *     the destination.
 *
 * For a qualified "DATASET::NAME" reference the field is named NAME; for a bare
 * tractography path it is named \a default_field_name. \a extra_properties are
 * stamped into the output header for the copy/rewrite cases (e.g. "SIFT2_mu"). */
void export_sidecar_column(const SidecarReference &reference,
                           const std::filesystem::path &input_path,
                           const Eigen::Array<default_type, Eigen::Dynamic, 1> &values,
                           std::string_view default_field_name,
                           const std::vector<std::pair<std::string, std::string>> &extra_properties = {});

} // namespace MR::DWI::Tractography
