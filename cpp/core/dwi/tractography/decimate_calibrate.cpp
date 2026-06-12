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
#include <cmath>
#include <iomanip>
#include <sstream>

#include "app.h"
#include "exception.h"
#include "file/ofstream.h"
#include "mrtrix.h"
#include "progressbar.h"
#include "thread_queue.h"

#include "dwi/tractography/distance.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/resampling/decimate_fast.h"
#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography {

namespace {

using value_type = float;

//! One worker's output for a single streamline: the Hausdorff distance (mm) at each swept mu, plus
//!   the vertex counts shared across all mu (the input count and, per mu, the output count).
struct StreamlineRecord {
  size_t index{0};
  uint64_t input_vertices{0};
  std::vector<value_type> hausdorff_mm;  // one per mu (NaN when undefined)
  std::vector<uint64_t> output_vertices; // one per mu
};

//! Per-streamline worker: decimates against every mu and measures the spline Hausdorff distance.
/*! Reuses the shipped \c Resampling::DecimateFast (constructed exactly as \c tckresample does, via
 *  the density knob mu) and the stage-3.5 \c hausdorff verbatim. Each worker owns its own decimator
 *  instances so the operation is thread-safe. */
class Worker {
public:
  explicit Worker(const std::vector<default_type> &mu_values) {
    decimators.reserve(mu_values.size());
    for (const default_type mu : mu_values)
      decimators.emplace_back(mu);
  }

  bool operator()(const Streamline<value_type> &in, StreamlineRecord &out) const {
    out.index = in.get_index();
    out.input_vertices = in.size();
    out.hausdorff_mm.assign(decimators.size(), std::numeric_limits<value_type>::quiet_NaN());
    out.output_vertices.assign(decimators.size(), 0);
    Streamline<value_type> decimated;
    for (size_t i = 0; i != decimators.size(); ++i) {
      decimators[i](in, decimated);
      out.output_vertices[i] = decimated.size();
      if (in.size() < 2 || decimated.size() < 2)
        continue;
      const HausdorffResult result = hausdorff(in, decimated);
      if (std::isfinite(result.distance))
        out.hausdorff_mm[i] = static_cast<value_type>(result.distance);
    }
    return true;
  }

private:
  std::vector<Resampling::DecimateFast> decimators;
};

//! Sink that accumulates, per mu, the finite per-streamline distances and the vertex-count ratios.
/*! Holds every finite distance (one float per streamline per mu) so that the percentiles can be
 *  computed exactly via \c std::nth_element. See \c decimate_calibrate documentation for the memory
 *  cost note. */
class Accumulator {
public:
  Accumulator(const std::vector<default_type> &mu_values, const size_t num_tracks)
      : mu_values(mu_values),
        distances(mu_values.size()),
        sum_ratio(mu_values.size(), 0.0),
        ratio_count(mu_values.size(), 0),
        progress("calibrating decimation", num_tracks) {
    for (auto &v : distances)
      v.reserve(num_tracks);
  }

  bool operator()(const StreamlineRecord &record) {
    for (size_t i = 0; i != mu_values.size(); ++i) {
      if (std::isfinite(record.hausdorff_mm[i]))
        distances[i].push_back(record.hausdorff_mm[i]);
      if (record.input_vertices > 0) {
        sum_ratio[i] +=
            static_cast<default_type>(record.output_vertices[i]) / static_cast<default_type>(record.input_vertices);
        ++ratio_count[i];
      }
    }
    ++progress;
    return true;
  }

  std::vector<CalibrationRow> finalise() {
    std::vector<CalibrationRow> rows;
    rows.reserve(mu_values.size());
    for (size_t i = 0; i != mu_values.size(); ++i) {
      CalibrationRow row;
      row.mu = mu_values[i];
      row.count = distances[i].size();
      row.percentiles_mm = compute_percentiles(distances[i]);
      row.compression = ratio_count[i] > 0 ? sum_ratio[i] / static_cast<default_type>(ratio_count[i]) : NaN;
      rows.push_back(row);
    }
    return rows;
  }

private:
  const std::vector<default_type> &mu_values;
  std::vector<std::vector<value_type>> distances;
  std::vector<default_type> sum_ratio;
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

} // namespace

std::vector<CalibrationRow> decimate_calibrate(const std::filesystem::path &path,
                                               const std::vector<default_type> &mu_values) {
  if (mu_values.empty())
    throw Exception("no mu values provided for decimation calibration");
  for (const default_type mu : mu_values) {
    if (!(mu > 0.0))
      throw Exception("invalid mu value " + str(mu) +
                      " for decimation calibration;" //
                      " every value must be strictly positive");
  }

  Properties properties;
  Reader<value_type> reader(path, properties);
  size_t num_tracks = 0;
  if (properties.find("count") != properties.end())
    num_tracks = to<size_t>(properties["count"]);

  Worker worker(mu_values);
  Accumulator accumulator(mu_values, num_tracks);
  Thread::run_queue(reader,
                    Thread::batch(Streamline<value_type>()),
                    Thread::multi(worker),
                    Thread::batch(StreamlineRecord()),
                    accumulator);

  return accumulator.finalise();
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

} // namespace

std::string calibration_table(const std::vector<CalibrationRow> &rows) {
  constexpr int width = 12;
  constexpr int precision = 5;
  std::ostringstream stream;
  stream << std::setw(width) << std::right << "mu";
  for (const default_type p : calibration_percentiles)
    stream << std::setw(width) << std::right << ("p" + percentile_label(p) + "(mm)");
  stream << std::setw(width) << std::right << "compress"
         << "\n";
  for (const CalibrationRow &row : rows) {
    stream << std::setw(width) << std::right << format_value(row.mu, precision);
    for (const default_type value : row.percentiles_mm)
      stream << std::setw(width) << std::right << format_value(value, precision);
    stream << std::setw(width) << std::right << format_value(row.compression, precision) << "\n";
  }
  return stream.str();
}

void calibration_save_csv(const std::vector<CalibrationRow> &rows, const std::filesystem::path &path) {
  constexpr int precision = 6;
  File::OFStream out(path, std::ios_base::out | std::ios_base::trunc);
  out << "# " << App::command_history_string << "\n";
  out << "mu";
  for (const default_type p : calibration_percentiles)
    out << ",p" << percentile_label(p) << "_mm";
  out << ",compression,count\n";
  for (const CalibrationRow &row : rows) {
    out << format_value(row.mu, precision);
    for (const default_type value : row.percentiles_mm)
      out << "," << format_value(value, precision);
    out << "," << format_value(row.compression, precision) << "," << str(row.count) << "\n";
  }
}

} // namespace MR::DWI::Tractography
