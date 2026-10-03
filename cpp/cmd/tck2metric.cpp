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

#include <filesystem>
#include <optional>
#include <vector>

#include "command.h"
#include "memory.h"
#include "progressbar.h"
#include "types.h"

#include "file/matrix.h"

#include "ordered_thread_queue.h"
#include "thread.h"

#include "dwi/tractography/curvature.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/weights.h"

using namespace MR;
using namespace App;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Compute one or more per-streamline metrics for a tractogram";

  DESCRIPTION
  + "Each requested metric is derived from a single serialised pass through the input tractogram,"
    " so that multiple metrics can be exported simultaneously without re-reading the data."

  + "At least one metric export option must be specified."

  + "Streamline length can be exported per-streamline to a vector file using the -length option,"
    " or summarised as a single mean value reported to stdout using the -mean_length option."

  + "Streamline curvature is estimated using a smooth arc-length-based local fit;"
    " the -mean_curvature option exports the per-streamline mean curvature to a vector file,"
    " whereas the -vertex_curvature option exports the per-vertex curvature to a track scalar (.tsf) file,"
    " the latter being written in the same order as the streamlines of the input tractogram.";

  ARGUMENTS
  + Argument ("tracks_in", "the input track file").type_tracks_in();

  OPTIONS

  + OptionGroup ("Options for exporting streamline length")

  + Option ("length", "export the length of each streamline to a vector file")
    + Argument ("path").type_file_out()

  + Option ("mean_length", "compute the mean streamline length and report it to stdout")

  + Option ("ignorezero", "do not generate a warning if the track file contains streamlines with zero length")

  + OptionGroup ("Options for exporting streamline curvature")

  + Option ("mean_curvature", "export the per-streamline mean curvature (1/mm) to a vector file")
    + Argument ("path").type_file_out()

  + Option ("vertex_curvature", "export the per-vertex curvature (1/mm) to a track scalar (.tsf) file")
    + Argument ("path").type_file_out()

  + Tractography::TrackWeightsInOption;

}
// clang-format on

using value_type = float;

// Per-streamline metrics passed through the ordered queue from Worker to Receiver
struct Metrics {
  size_t index{size_t(-1)};
  value_type weight{1.0F};
  value_type length{NaNF};
  value_type mean_curvature{NaNF};
  TrackScalar<value_type> vertex_curvature;
};

// Computes the requested per-streamline metrics; replicated across threads
class Worker {
public:
  Worker(const bool compute_curvature, const CurvatureConfig &config)
      : compute_curvature(compute_curvature), config(config) {}

  bool operator()(const Streamline<value_type> &tck, Metrics &out) const {
    out.index = tck.get_index();
    out.weight = tck.weight;
    out.length = Tractography::length(tck, Tractography::LengthMethod::CHORD);
    out.mean_curvature = NaNF;
    out.vertex_curvature.clear();
    out.vertex_curvature.set_index(tck.get_index());
    if (compute_curvature) {
      const std::vector<default_type> kappa = Tractography::curvature(tck, config);
      out.vertex_curvature.resize(kappa.size());
      default_type sum = 0.0;
      for (size_t i = 0; i != kappa.size(); ++i) {
        out.vertex_curvature[i] = static_cast<value_type>(kappa[i]);
        sum += kappa[i];
      }
      if (!kappa.empty())
        out.mean_curvature = static_cast<value_type>(sum / static_cast<default_type>(kappa.size()));
    }
    return true;
  }

private:
  const bool compute_curvature;
  CurvatureConfig config;
};

// Accumulates streamline-length summary statistics and streams ordered per-streamline outputs to file
class Receiver {
public:
  Receiver(const Properties &properties,
           const bool collect_length,
           const bool collect_mean_curvature,
           const std::optional<std::filesystem::path> &vertex_curvature_path,
           const size_t num_tracks)
      : collect_length(collect_length),
        collect_mean_curvature(collect_mean_curvature),
        progress("Computing streamline metrics", num_tracks) {
    if (vertex_curvature_path.has_value())
      tsf.reset(new ScalarWriter<value_type>(*vertex_curvature_path, properties));
  }

  Receiver(const Receiver &) = delete;

  bool operator()(const Metrics &in) {
    // The ordered queue must deliver streamlines in input order
    assert(in.index == count);
    ++count;
    if (std::isfinite(in.length)) {
      sum_lengths += in.weight * in.length;
      sum_weights += in.weight;
      if (in.length == 0.0F)
        ++zero_length_streamlines;
    } else {
      ++empty_streamlines;
    }
    if (collect_length)
      lengths.push_back(in.length);
    if (collect_mean_curvature)
      mean_curvatures.push_back(in.mean_curvature);
    if (tsf)
      (*tsf)(in.vertex_curvature);
    ++progress;
    return true;
  }

  size_t streamline_count() const { return count; }
  size_t empty_count() const { return empty_streamlines; }
  size_t zero_length_count() const { return zero_length_streamlines; }
  default_type sum_length() const { return sum_lengths; }
  default_type sum_weight() const { return sum_weights; }
  const std::vector<value_type> &length_values() const { return lengths; }
  const std::vector<value_type> &mean_curvature_values() const { return mean_curvatures; }

private:
  const bool collect_length;
  const bool collect_mean_curvature;
  size_t count = 0;
  size_t empty_streamlines = 0;
  size_t zero_length_streamlines = 0;
  default_type sum_lengths = 0.0;
  default_type sum_weights = 0.0;
  std::vector<value_type> lengths;
  std::vector<value_type> mean_curvatures;
  std::unique_ptr<ScalarWriter<value_type>> tsf;
  ProgressBar progress;
};

void run() {
  Properties properties;
  Reader<value_type> reader(argument[0], properties);

  size_t header_count = 0;
  if (properties.find("count") != properties.end())
    header_count = to<size_t>(properties["count"]);

  std::optional<std::filesystem::path> length_path;
  auto opt = get_options("length");
  if (!opt.empty())
    length_path = std::filesystem::path(opt[0][0]);

  const bool mean_length = !get_options("mean_length").empty();

  std::optional<std::filesystem::path> mean_curvature_path;
  opt = get_options("mean_curvature");
  if (!opt.empty())
    mean_curvature_path = std::filesystem::path(opt[0][0]);

  std::optional<std::filesystem::path> vertex_curvature_path;
  opt = get_options("vertex_curvature");
  if (!opt.empty()) {
    vertex_curvature_path = std::filesystem::path(opt[0][0]);
    if (!Path::has_suffix(*vertex_curvature_path, ".tsf"))
      throw Exception("Per-vertex curvature must be exported to a track scalar (.tsf) file");
  }

  if (!length_path.has_value() && !mean_length && !mean_curvature_path.has_value() &&
      !vertex_curvature_path.has_value())
    throw Exception("At least one metric export option must be specified");

  const bool compute_curvature = mean_curvature_path.has_value() || vertex_curvature_path.has_value();
  CurvatureConfig config;
  if (compute_curvature)
    configure_from_properties(config, properties);

  Worker worker(compute_curvature, config);
  Receiver receiver(
      properties, length_path.has_value(), mean_curvature_path.has_value(), vertex_curvature_path, header_count);
  Thread::run_ordered_queue(
      reader, Thread::batch(Streamline<value_type>()), Thread::multi(worker), Thread::batch(Metrics()), receiver);

  const size_t count = receiver.streamline_count();

  if (get_options("ignorezero").empty() && (receiver.empty_count() != 0 || receiver.zero_length_count() != 0)) {
    std::string s("read");
    if (receiver.empty_count() != 0) {
      s += " " + str(receiver.empty_count()) + " empty streamlines";
      if (receiver.zero_length_count() != 0)
        s += " and";
    }
    if (receiver.zero_length_count() != 0)
      s += " " + str(receiver.zero_length_count()) + " streamlines with zero length (one vertex only)";
    WARN(s);
  }
  if (count != header_count)
    WARN("expected " + str(header_count) + " tracks according to header; read " + str(count));

  if (mean_length) {
    const default_type sum_weights = receiver.sum_weight();
    const float value = sum_weights != 0.0 ? static_cast<float>(receiver.sum_length() / sum_weights) : NaNF;
    std::cout << str(value) << "\n";
  }

  if (length_path.has_value())
    File::Matrix::save_vector(receiver.length_values(), *length_path);

  if (mean_curvature_path.has_value())
    File::Matrix::save_vector(receiver.mean_curvature_values(), *mean_curvature_path);
}
