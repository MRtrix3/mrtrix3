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

#include "dwi/tractography/decimate_calibrate.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <type_traits>

#include "app.h"
#include "exception.h"
#include "file/ofstream.h"
#include "mrtrix.h"
#include "progressbar.h"
#include "thread_queue.h"

#include "dwi/tractography/curvature.h"
#include "dwi/tractography/distance.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/resampling/decimate_fast.h"
#include "dwi/tractography/resampling/decimate_slow.h"
#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography {

namespace {

using value_type = float;

//! One worker's output for a single streamline: per swept parameter value, the Hausdorff distance
//!   (mm), the output vertex count and the decimation time (s); plus the shared input vertex count.
struct StreamlineRecord {
  size_t index{0};
  uint64_t input_vertices{0};
  std::vector<value_type> hausdorff_mm;      // one per parameter (NaN when undefined)
  std::vector<uint64_t> output_vertices;     // one per parameter
  std::vector<default_type> decimation_time; // seconds spent decimating, one per parameter
};

//! Inject the (possibly metadata-derived) curvature configuration into the decimator.
/*! Only the fast decimator's a-priori vertex placement consults the curvature estimator; the slow
 *  decimator is purely geometric and exposes no such hook, so for it this is a deliberate no-op.
 *  Overloading keeps the templated worker free of decimator-specific branches. */
inline void apply_curvature_config(Resampling::DecimateFast &decimator, const CurvatureConfig &config) {
  decimator.set_curvature_config(config);
}
inline void apply_curvature_config(Resampling::DecimateSlow &, const CurvatureConfig &) {}

//! Per-streamline worker: decimates against every swept parameter and measures the Hausdorff error.
/*! Reuses the shipped decimator \c Decimator (\c Resampling::DecimateFast or \c DecimateSlow,
 *  constructed exactly as \c tckresample does) and the stage-3.5 \c hausdorff verbatim. Each worker
 *  owns its own decimator instances so the operation is thread-safe. The decimation cost is timed
 *  around the decimator call alone (the Hausdorff measurement, a calibration-only diagnostic, is
 *  excluded). */
template <class Decimator> class Worker {
public:
  Worker(const std::vector<default_type> &parameter_values, const CurvatureConfig &curvature_config) {
    // The curvature configuration (possibly carrying a metadata-derived sub-step adjustment) is
    //   shared by the fast decimator and the Hausdorff probe-spacing rule so the calibration is
    //   measured consistently with the way the curvature estimator is actually applied.
    hausdorff_config.curvature = curvature_config;
    decimators.reserve(parameter_values.size());
    for (const default_type value : parameter_values) {
      decimators.emplace_back(value);
      apply_curvature_config(decimators.back(), curvature_config);
    }
  }

  bool operator()(const Streamline<value_type> &in, StreamlineRecord &out) const {
    out.index = in.get_index();
    out.input_vertices = in.size();
    out.hausdorff_mm.assign(decimators.size(), std::numeric_limits<value_type>::quiet_NaN());
    out.output_vertices.assign(decimators.size(), 0);
    out.decimation_time.assign(decimators.size(), 0.0);
    Streamline<value_type> decimated;
    for (size_t i = 0; i != decimators.size(); ++i) {
      const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
      decimators[i](in, decimated);
      const std::chrono::steady_clock::time_point stop = std::chrono::steady_clock::now();
      out.decimation_time[i] = std::chrono::duration<default_type>(stop - start).count();
      out.output_vertices[i] = decimated.size();
      if (in.size() < 2 || decimated.size() < 2)
        continue;
      const HausdorffResult result = hausdorff(in, decimated, hausdorff_config);
      if (std::isfinite(result.distance))
        out.hausdorff_mm[i] = static_cast<value_type>(result.distance);
    }
    return true;
  }

private:
  std::vector<Decimator> decimators;
  HausdorffConfig hausdorff_config;
};

//! Sink that accumulates, per parameter value, the finite per-streamline distances, the vertex-count
//!   ratios and output counts, and the summed decimation time.
/*! Holds every finite distance (one float per streamline per parameter) so that the percentiles can
 *  be computed exactly via \c std::nth_element. See \c decimate_calibrate documentation for the
 *  memory cost note. */
class Accumulator {
public:
  Accumulator(const std::vector<default_type> &parameter_values, const size_t num_tracks)
      : parameter_values(parameter_values),
        distances(parameter_values.size()),
        sum_ratio(parameter_values.size(), 0.0),
        sum_output_vertices(parameter_values.size(), 0.0),
        sum_time(parameter_values.size(), 0.0),
        ratio_count(parameter_values.size(), 0),
        progress("calibrating decimation", num_tracks) {
    for (auto &v : distances)
      v.reserve(num_tracks);
  }

  bool operator()(const StreamlineRecord &record) {
    for (size_t i = 0; i != parameter_values.size(); ++i) {
      if (std::isfinite(record.hausdorff_mm[i]))
        distances[i].push_back(record.hausdorff_mm[i]);
      // Every streamline is decimated for every parameter, so the cost is summed unconditionally.
      sum_time[i] += record.decimation_time[i];
      if (record.input_vertices > 0) {
        sum_ratio[i] +=
            static_cast<default_type>(record.output_vertices[i]) / static_cast<default_type>(record.input_vertices);
        sum_output_vertices[i] += static_cast<default_type>(record.output_vertices[i]);
        ++ratio_count[i];
      }
    }
    ++progress;
    return true;
  }

  std::vector<CalibrationRow> finalise() {
    std::vector<CalibrationRow> rows;
    rows.reserve(parameter_values.size());
    for (size_t i = 0; i != parameter_values.size(); ++i) {
      CalibrationRow row;
      row.parameter = parameter_values[i];
      row.count = distances[i].size();
      row.percentiles_mm = compute_percentiles(distances[i]);
      const default_type denominator = static_cast<default_type>(ratio_count[i]);
      row.compression = ratio_count[i] > 0 ? sum_ratio[i] / denominator : NaN;
      row.mean_output_vertices = ratio_count[i] > 0 ? sum_output_vertices[i] / denominator : NaN;
      row.decimation_time_s = sum_time[i];
      rows.push_back(row);
    }
    return rows;
  }

private:
  const std::vector<default_type> &parameter_values;
  std::vector<std::vector<value_type>> distances;
  std::vector<default_type> sum_ratio;
  std::vector<default_type> sum_output_vertices;
  std::vector<default_type> sum_time;
  std::vector<uint64_t> ratio_count;
  ProgressBar progress;

  //! Exact percentiles of a (modifiable) sample via partial selection; empty sample yields NaN.
  static std::array<default_type, calibration_percentiles.size()> compute_percentiles(std::vector<value_type> &sample) {
    std::array<default_type, calibration_percentiles.size()> result;
    result.fill(NaN);
    if (sample.empty())
      return result;
    for (size_t p = 0; p != calibration_percentiles.size(); ++p) {
      // Nearest-rank percentile: rank in [1, n], clamped, zero-based for indexing.
      const default_type rank = std::ceil(calibration_percentiles[p] / 100.0 * sample.size());
      size_t k = rank < 1.0 ? 0 : static_cast<size_t>(rank) - 1;
      k = std::min(k, sample.size() - 1);
      std::nth_element(sample.begin(), sample.begin() + k, sample.end());
      result[p] = static_cast<default_type>(sample[k]);
    }
    return result;
  }
};

//! Run the full source -> multi-worker -> sink sweep for one concrete decimator type.
/*! Factored out so the two algorithms share the identical thread-queue plumbing and differ only in
 *  the decimator instantiated (and, via \c apply_curvature_config, whether the curvature hook is
 *  used). */
template <class Decimator>
std::vector<CalibrationRow> run_sweep(const std::filesystem::path &path,
                                      const std::vector<default_type> &parameter_values) {
  Properties properties;
  Reader<value_type> reader(path, properties);
  size_t num_tracks = 0;
  if (properties.find("count") != properties.end())
    num_tracks = to<size_t>(properties["count"]);

  // Derive any curvature-scale adjustment from the generator metadata once (warns at most once).
  CurvatureConfig curvature_config;
  configure_from_properties(curvature_config, properties);

  Worker<Decimator> worker(parameter_values, curvature_config);
  Accumulator accumulator(parameter_values, num_tracks);
  Thread::run_queue(reader,
                    Thread::batch(Streamline<value_type>()),
                    Thread::multi(worker),
                    Thread::batch(StreamlineRecord()),
                    accumulator);

  return accumulator.finalise();
}

} // namespace

std::vector<CalibrationRow> decimate_calibrate(const std::filesystem::path &path,
                                               const std::vector<default_type> &parameter_values,
                                               const DecimateAlgorithm algorithm) {
  if (parameter_values.empty())
    throw Exception("no parameter values provided for decimation calibration");
  for (const default_type value : parameter_values) {
    if (!(value > 0.0))
      throw Exception("invalid decimation calibration value " + str(value) +
                      ";" //
                      " every value must be strictly positive");
  }

  switch (algorithm) {
  case DecimateAlgorithm::Fast:
    return run_sweep<Resampling::DecimateFast>(path, parameter_values);
  case DecimateAlgorithm::Slow:
    return run_sweep<Resampling::DecimateSlow>(path, parameter_values);
  }
  assert(false);
  return {};
}

namespace {

//! Format a percentile (e.g. 99.9) without floating-point display artefacts or trailing zeros.
std::string percentile_label(const default_type p) {
  std::ostringstream stream;
  stream << std::setprecision(4) << p;
  return stream.str();
}

//! Format a numeric value with a fixed display precision (NaN renders as "nan").
std::string format_value(const default_type value, const int precision) {
  if (!std::isfinite(value))
    return "nan";
  return str(value, precision);
}

//! Base header label for the leading swept-parameter column (mu vs. tolerance), per algorithm.
std::string parameter_column_label(const DecimateAlgorithm algorithm) {
  switch (algorithm) {
  case DecimateAlgorithm::Fast:
    return "mu";
  case DecimateAlgorithm::Slow:
    return "tolerance";
  }
  assert(false);
  return "parameter";
}

} // namespace

std::string calibration_table(const std::vector<CalibrationRow> &rows, const DecimateAlgorithm algorithm) {
  constexpr int width = 12;
  constexpr int precision = 5;
  // For the slow algorithm the parameter is itself a tolerance in mm; flag the unit in the header.
  const std::string parameter_header =
      parameter_column_label(algorithm) + (algorithm == DecimateAlgorithm::Slow ? "(mm)" : "");
  std::ostringstream stream;
  stream << std::setw(width) << std::right << parameter_header;
  for (const default_type p : calibration_percentiles)
    stream << std::setw(width) << std::right << ("p" + percentile_label(p) + "(mm)");
  stream << std::setw(width) << std::right << "compress"  //
         << std::setw(width) << std::right << "out_verts" //
         << std::setw(width) << std::right << "time(s)"
         << "\n";
  for (const CalibrationRow &row : rows) {
    stream << std::setw(width) << std::right << format_value(row.parameter, precision);
    for (const default_type value : row.percentiles_mm)
      stream << std::setw(width) << std::right << format_value(value, precision);
    stream << std::setw(width) << std::right << format_value(row.compression, precision)          //
           << std::setw(width) << std::right << format_value(row.mean_output_vertices, precision) //
           << std::setw(width) << std::right << format_value(row.decimation_time_s, precision) << "\n";
  }
  return stream.str();
}

void calibration_save_csv(const std::vector<CalibrationRow> &rows,
                          const DecimateAlgorithm algorithm,
                          const std::filesystem::path &path) {
  constexpr int precision = 6;
  // For the slow algorithm the parameter is a tolerance in mm; encode the unit in the column name.
  const std::string parameter_header =
      parameter_column_label(algorithm) + (algorithm == DecimateAlgorithm::Slow ? "_mm" : "");
  File::OFStream out(path, std::ios_base::out | std::ios_base::trunc);
  out << "# " << App::command_history_string << "\n";
  out << parameter_header;
  for (const default_type p : calibration_percentiles)
    out << ",p" << percentile_label(p) << "_mm";
  out << ",compression,mean_output_vertices,decimation_time_s,count\n";
  for (const CalibrationRow &row : rows) {
    out << format_value(row.parameter, precision);
    for (const default_type value : row.percentiles_mm)
      out << "," << format_value(value, precision);
    out << "," << format_value(row.compression, precision)          //
        << "," << format_value(row.mean_output_vertices, precision) //
        << "," << format_value(row.decimation_time_s, precision)    //
        << "," << str(row.count) << "\n";
  }
}

} // namespace MR::DWI::Tractography
