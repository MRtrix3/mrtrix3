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
#include <string_view>
#include <vector>

namespace MR::DWI::Tractography {

//! \brief The reserved name of the embedded streamline-selection dps field (§2.5).
/*! tckedit and tcksift can record a per-streamline integer/bitwise selection
 * field alongside the output tractogram (Stage 12, step 3). The field carries
 * one value per OUTPUT streamline (1 = the streamline was selected into the
 * output); for tckedit vertex-masking, each fragment of a fragmented input
 * streamline is an output streamline and is assigned the selection value. */
constexpr std::string_view selection_dps_name = "selected";

//! \brief write a per-streamline selection dps field to a standalone file.
/*! Each entry of \a values is the selection value (a 0/1 flag, stored uint8) of
 * one output streamline, in output order. The data are written as a per-
 * streamline numerical column to \a path (a plain-text or ".npy" file, chosen by
 * extension), exactly as a standalone dps sidecar (§2.5/§2.7). This is the
 * embedded counterpart to the existing whole-input selection text file: where
 * that records the binary mask over the INPUT streamlines, this records the
 * (per-output) selection field that travels with the output tractogram. */
void write_selection_dps(const std::filesystem::path &path, const std::vector<uint8_t> &values);

} // namespace MR::DWI::Tractography
