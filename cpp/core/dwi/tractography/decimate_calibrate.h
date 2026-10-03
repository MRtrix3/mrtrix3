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

//! Which of the two tckresample decimation resamplers a calibration sweep exercises.
/*! \c Fast is the curvature-adaptive single-pass \c Resampling::DecimateFast, whose swept parameter
 *  is the dimensionless density knob mu (output vertices per unit curvature-weighted arc length;
 *  larger retains more vertices). \c Slow is the greedy knot-insertion \c Resampling::DecimateSlow,
 *  whose swept parameter is a deviation tolerance (mm): the reconstruction is bounded within that
 *  tolerance of the original spline by construction. The enumerator names (lower-cased) double as
 *  the user-facing \c -algorithm choices via \c Argument::type_choice<>. */
enum class DecimateAlgorithm { Fast, Slow };

//! The fixed set of Hausdorff-distribution percentiles reported per swept parameter value.
/*! Ordered ascending so that, on well-behaved data, the per-parameter reported values are themselves
 *  ascending. The 100th percentile is the per-streamline maximum Hausdorff distance. */
constexpr std::array<default_type, 6> calibration_percentiles{50.0, 75.0, 95.0, 99.0, 99.9, 100.0};

//! Per-parameter summary of one decimation calibration sweep.
/*! \c parameter is the swept decimation knob that produced this row: the density mu for the fast
 *  algorithm, the Hausdorff-distance tolerance (mm) for the slow one. \c percentiles_mm holds the
 *  measured symmetric spline-Hausdorff-distance percentiles (mm), in the order of
 *  \c calibration_percentiles, across all streamlines that contributed a finite distance; for the
 *  slow algorithm these verify that the achieved error respects the requested tolerance. \c
 *  compression is the mean output/input vertex ratio (dimensionless, in (0, 1]); smaller means more
 *  aggressive decimation. \c mean_output_vertices is the mean absolute output vertex count (the
 *  determinant of on-disk size). \c decimation_time_s is the summed time (s) spent inside the
 *  decimator across all contributing streamlines for this parameter (the resampling cost, excluding
 *  the Hausdorff measurement). \c count is the number of streamlines contributing a finite distance. */
struct CalibrationRow {
  default_type parameter;
  std::array<default_type, calibration_percentiles.size()> percentiles_mm;
  default_type compression;
  default_type mean_output_vertices;
  default_type decimation_time_s;
  uint64_t count;
};

//! Sweep a decimation resampler over a set of parameter values and summarise cost and error.
/*! For every streamline in \a path and every value in \a parameter_values, the selected \a algorithm
 *  decimator (the shipped \c Resampling::DecimateFast or \c Resampling::DecimateSlow, constructed
 *  exactly as \c tckresample does) is run and the stage-3.5 symmetric spline \c hausdorff distance
 *  (mm) between the original and decimated streamline is computed. The per-streamline distances are
 *  reduced, per parameter value, to the \c calibration_percentiles, alongside the mean output/input
 *  vertex ratio, the mean absolute output vertex count, and the summed decimation time.
 *
 *  Work is distributed over the multi-threading pool with the same source -> multi-worker -> sink
 *  thread-queue pattern as \c tckresample: each worker processes one streamline against the entire
 *  parameter set and emits a compact record, and the single sink accumulates those records. The
 *  reported time is the sum of per-call wall-clock durations (total decimator CPU work), so it is
 *  independent of the thread count and directly comparable across parameter values and algorithms.
 *
 *  \par Quantile strategy
 *  Percentiles are computed \b exactly: the sink retains every finite per-streamline distance (one
 *  \c float per streamline per parameter) and applies \c std::nth_element at the end. The transient
 *  memory cost is therefore O(num_streamlines * num_parameters * sizeof(float)); for a 10^6-streamline
 *  tractogram swept over ten values this is ~40 MB, which is acceptable for a calibration utility
 *  typically run on a representative subset. No streaming/approximate estimator is used, so the
 *  reported percentiles are not subject to binning error.
 *
 *  \param path              the input tractogram.
 *  \param parameter_values  the values to sweep; each must be strictly positive (validated).
 *  \param algorithm         which decimation resampler to exercise.
 *  \returns one \c CalibrationRow per parameter value, in the order of \a parameter_values. */
std::vector<CalibrationRow> decimate_calibrate(const std::filesystem::path &path,
                                               const std::vector<default_type> &parameter_values,
                                               DecimateAlgorithm algorithm);

//! Render a calibration sweep as a human-readable, monospaced table.
/*! Rows are parameter values; columns are the six Hausdorff percentiles (mm), the mean compression
 *  ratio, the mean output vertex count and the total decimation time (s). The \a algorithm selects
 *  the header label of the leading parameter column (mu vs. tolerance). */
std::string calibration_table(const std::vector<CalibrationRow> &rows, DecimateAlgorithm algorithm);

//! Write a calibration sweep to a CSV file (one row per parameter value; units in the header).
/*! The \a algorithm selects the header label of the leading parameter column (mu vs. tolerance_mm). */
void calibration_save_csv(const std::vector<CalibrationRow> &rows,
                          DecimateAlgorithm algorithm,
                          const std::filesystem::path &path);

} // namespace MR::DWI::Tractography
