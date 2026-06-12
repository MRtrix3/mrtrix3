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
#include <vector>

#include "types.h"

#include "dwi/tractography/file.h"

namespace MR::DWI::Tractography {

//! The fixed set of Hausdorff-distribution percentiles reported per mu.
/*! Ordered ascending so that, on well-behaved data, the per-mu reported values are themselves
 *  ascending. The 100th percentile is the per-streamline maximum Hausdorff distance. */
constexpr std::array<default_type, 6> calibration_percentiles{50.0, 75.0, 95.0, 99.0, 99.9, 100.0};

//! Per-mu summary of one decimation calibration sweep.
/*! \c mu is the fast-decimator density knob that produced this row. \c percentiles_mm holds the
 *  Hausdorff-distance percentiles (mm), in the order of \c calibration_percentiles, across all
 *  streamlines that contributed a finite distance. \c compression is the mean output/input vertex
 *  ratio (dimensionless, in (0, 1]); smaller means more aggressive decimation. \c count is the
 *  number of contributing streamlines. */
struct CalibrationRow {
  default_type mu;
  std::array<default_type, calibration_percentiles.size()> percentiles_mm;
  default_type compression;
  uint64_t count;
};

//! Sweep the fast decimator over a set of mu values and summarise the spline-Hausdorff error.
/*! For every streamline in \a path and every value in \a mu_values, the shipped
 *  \c Resampling::DecimateFast decimator is run and the stage-3.5 symmetric spline \c hausdorff
 *  distance (mm) between the original and decimated streamline is computed. The per-streamline
 *  distances are reduced, per mu, to the \c calibration_percentiles plus the mean output/input
 *  vertex ratio.
 *
 *  Work is distributed over the multi-threading pool with the same source -> multi-worker -> sink
 *  thread-queue pattern as \c tckresample: each worker processes one streamline against the entire
 *  mu set and emits a compact record, and the single sink accumulates those records.
 *
 *  \par Quantile strategy
 *  Percentiles are computed \b exactly: the sink retains every finite per-streamline distance (one
 *  \c float per streamline per mu) and applies \c std::nth_element at the end. The transient memory
 *  cost is therefore O(num_streamlines * num_mu * sizeof(float)); for a 10^6-streamline tractogram
 *  swept over ten mu values this is ~40 MB, which is acceptable for a calibration utility typically
 *  run on a representative subset. No streaming/approximate estimator is used, so the reported
 *  percentiles are not subject to binning error.
 *
 *  \param path        the input tractogram.
 *  \param mu_values   the mu values to sweep; each must be strictly positive (validated).
 *  \returns one \c CalibrationRow per mu, in the order of \a mu_values. */
std::vector<CalibrationRow> decimate_calibrate(const std::filesystem::path &path,
                                               const std::vector<default_type> &mu_values);

//! Render a calibration sweep as a human-readable, monospaced table.
/*! Rows are mu; columns are the six Hausdorff percentiles (mm) and the mean compression ratio. */
std::string calibration_table(const std::vector<CalibrationRow> &rows);

//! Write a calibration sweep to a CSV file (one row per mu; mm units in the header).
void calibration_save_csv(const std::vector<CalibrationRow> &rows, const std::filesystem::path &path);

} // namespace MR::DWI::Tractography
