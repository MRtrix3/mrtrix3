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

#include "command.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/resampling/arc.h"
#include "dwi/tractography/resampling/downsampler.h"
#include "dwi/tractography/resampling/endpoints.h"
#include "dwi/tractography/resampling/fixed_num_points.h"
#include "dwi/tractography/resampling/fixed_step_size.h"
#include "dwi/tractography/resampling/resampling.h"
#include "dwi/tractography/resampling/upsampler.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/weights.h"
#include "image.h"
#include "math/math.h"
#include "ordered_thread_queue.h"
#include "thread.h"

#include <filesystem>

using namespace MR;
using namespace App;
using namespace DWI::Tractography;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)"
           " and J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Resample each streamline in a track file to a new set of vertices";

  DESCRIPTION
  + "It is necessary to specify precisely ONE of the command-line options"
    " for controlling how this resampling takes place;"
    " this may be either increasing or decreasing the number of samples along each streamline,"
    " or may involve changing the positions of the samples according to some specified trajectory."

  + "Note that because the length of a streamline is calculated"
    " based on the sums of distances between adjacent vertices,"
    " resampling a streamline to a new set of vertices will typically"
    " change the quantified length of that streamline;"
    " the magnitude of the difference will typically depend on"
    " the discrepancy in the number of vertices,"
    " with less vertices leading to a shorter length"
    " (due to taking chordal lengths of curved trajectories)."

  + "Per-vertex (data-per-vertex) sidecar data, supplied as a track scalar file (.tsf)"
    " via the -tsf_in option, are updated to correspond to the output vertices."
    " For the vertex-subset-preserving modes (-downsample, -endpoints),"
    " each scalar is sub-sampled to the retained vertices."
    " For the interpolating modes (-upsample, -step_size, -num_points, -line, -arc),"
    " which invent new vertex positions, the per-vertex data cannot meaningfully be carried"
    " and are dropped (with a warning);"
    " in that circumstance the -tsf_out option has no effect."
    " Per-streamline weights (-tck_weights_in/out) pass through unchanged in every mode.";

  ARGUMENTS
  + Argument ("in_tracks", "the input track file").type_tracks_in()
  + Argument ("out_tracks", "the output resampled tracks").type_tracks_out();

  OPTIONS
    + Resampling::ResampleOption

    + OptionGroup ("Options for handling per-vertex (data-per-vertex) sidecar data")
    + Option ("tsf_in", "an input track scalar file (.tsf), one scalar per vertex of the input tractogram,"
                        " to be resampled to correspond to the output vertices"
                        " (only the vertex-subset modes -downsample / -endpoints preserve it;"
                        " interpolating modes drop it with a warning)")
      + Argument ("path").type_file_in()
    + Option ("tsf_out", "the output track scalar file (.tsf) corresponding to -tsf_in"
                         " (ignored for the interpolating modes, which drop per-vertex data)")
      + Argument ("path").type_file_out()

    + OptionGroup ("Options for handling per-streamline (data-per-streamline) weights")
    + TrackWeightsInOption
    + TrackWeightsOutOption;

    // TODO Resample according to an exemplar streamline
}
// clang-format on

using value_type = float;

//! \brief A streamline plus its (optional) parallel per-vertex scalar (.tsf).
/*! The composite queue item used when tckresample is asked to carry per-vertex
 * sidecar data (the -tsf_in option). When no .tsf is coupled, \c scalar is left
 * empty and the item degrades to a plain streamline. */
struct CoupledItem {
  Streamline<value_type> streamline;
  TrackScalar<value_type> scalar;
  void clear() {
    streamline.clear();
    scalar.clear();
  }
};

// ---------------------------------------------------------------------------
// Plain (no per-vertex sidecar) pipeline: the original behaviour.
// ---------------------------------------------------------------------------

class Worker {
public:
  Worker(const std::unique_ptr<Resampling::Base> &in) : resampler(in->clone()) {}

  Worker(const Worker &that) : resampler(that.resampler->clone()) {}

  bool operator()(const Streamline<value_type> &in, Streamline<value_type> &out) const {
    (*resampler)(in, out);
    return true;
  }

private:
  std::unique_ptr<Resampling::Base> resampler;
};

class Receiver {
public:
  Receiver(const std::filesystem::path &path, const Properties &properties)
      : writer(path, properties), progress("resampling streamlines") {}

  bool operator()(const Streamline<value_type> &tck) {
    auto progress_message = [&]() {
      return "resampling streamlines (count: " + str(writer.count) +
             ", skipped: " + str(writer.total_count - writer.count) + ")";
    };
    writer(tck);
    progress.set_text(progress_message());
    return true;
  }

private:
  Writer<value_type> writer;
  ProgressBar progress;
};

// ---------------------------------------------------------------------------
// Coupled (per-vertex sidecar) pipeline: -tsf_in / -tsf_out.
// ---------------------------------------------------------------------------

//! \brief reads a streamline and its parallel .tsf scalar in lock-step.
class CoupledLoader {
public:
  CoupledLoader(const std::filesystem::path &tracks_path,
                Properties &tck_properties,
                const std::filesystem::path &tsf_path,
                Properties &tsf_properties)
      : tck_reader(tracks_path, tck_properties), tsf_reader(tsf_path, tsf_properties) {}

  bool operator()(CoupledItem &item) {
    item.clear();
    if (!tck_reader(item.streamline))
      return false;
    if (!tsf_reader(item.scalar))
      throw Exception("Track scalar file exhausted before the tractogram;"
                      " the two do not contain the same number of streamlines");
    if (item.scalar.size() != item.streamline.size())
      throw Exception("Streamline " + str(item.streamline.get_index()) +                   //
                      " has " + str(item.streamline.size()) + " vertices but its scalar" + //
                      " sequence has " + str(item.scalar.size()) + " entries");            //
    return true;
  }

private:
  Reader<value_type> tck_reader;
  ScalarReader<value_type> tsf_reader;
};

//! \brief resamples a streamline and sub-samples its scalar to the output vertices.
class CoupledWorker {
public:
  CoupledWorker(const std::unique_ptr<Resampling::Base> &in) : resampler(in->clone()) {}
  CoupledWorker(const CoupledWorker &that) : resampler(that.resampler->clone()) {}

  bool operator()(const CoupledItem &in, CoupledItem &out) const {
    out.clear();
    (*resampler)(in.streamline, out.streamline);
    out.scalar.set_index(in.streamline.get_index());
    // The subset modes (downsample/endpoints) retain a subset of the input
    //   vertices; sub-sample the per-vertex scalar to exactly those vertices.
    //   The interpolating modes drop per-vertex data (handled by the receiver),
    //   so leave out.scalar empty here.
    if (resampler->preserves_vertex_subset()) {
      for (const size_t i : resampler->retained_indices(in.streamline))
        out.scalar.push_back(in.scalar[i]);
    }
    return true;
  }

private:
  std::unique_ptr<Resampling::Base> resampler;
};

//! \brief writes the resampled streamline and (for subset modes) its scalar.
class CoupledReceiver {
public:
  CoupledReceiver(const std::filesystem::path &tracks_path,
                  const Properties &tck_properties,
                  const std::optional<std::filesystem::path> &tsf_path)
      : writer(tracks_path, tck_properties), progress("resampling streamlines") {
    // The output .tsf must inherit the output tractogram's properties (notably
    //   its freshly-stamped "timestamp") so that the pair is mutually
    //   consistent, rather than carrying the input .tsf's timestamp.
    if (tsf_path.has_value())
      tsf_writer.reset(new ScalarWriter<value_type>(*tsf_path, tck_properties));
  }

  bool operator()(const CoupledItem &item) {
    writer(item.streamline);
    if (tsf_writer)
      (*tsf_writer)(item.scalar);
    auto progress_message = [&]() {
      return "resampling streamlines (count: " + str(writer.count) +
             ", skipped: " + str(writer.total_count - writer.count) + ")";
    };
    progress.set_text(progress_message());
    return true;
  }

private:
  Writer<value_type> writer;
  std::unique_ptr<ScalarWriter<value_type>> tsf_writer;
  ProgressBar progress;
};

void run() {
  Properties properties;

  const std::unique_ptr<Resampling::Base> resampler(Resampling::get_resampler());

  auto tsf_in = get_optional<std::filesystem::path>("tsf_in");
  auto tsf_out = get_optional<std::filesystem::path>("tsf_out");

  if (!tsf_in.has_value()) {
    if (tsf_out.has_value())
      throw Exception("The -tsf_out option requires the -tsf_in option");
    // Original (vertices-only) pipeline.
    Reader<value_type> read(argument[0], properties);
    Worker worker(resampler);
    Receiver receiver(argument[1], properties);
    Thread::run_ordered_queue(read,
                              Thread::batch(Streamline<value_type>()),
                              Thread::multi(worker),
                              Thread::batch(Streamline<value_type>()),
                              receiver);
    return;
  }

  // Coupled per-vertex sidecar pipeline.
  const bool drop_dpv = !resampler->preserves_vertex_subset();
  if (drop_dpv) {
    // D8 drop: a single explanatory warning; the output omits per-vertex data.
    WARN("Resampling mode interpolates new vertex positions;"
         " per-vertex (data-per-vertex) sidecar data from \"" +
         tsf_in->string() + "\"" + " cannot be carried to the output and will be dropped");
    if (tsf_out.has_value()) {
      WARN("The -tsf_out option is ignored for interpolating resampling modes");
      tsf_out = std::nullopt;
    }
  }

  Properties tsf_in_properties;
  CoupledLoader loader(argument[0], properties, *tsf_in, tsf_in_properties);
  CoupledWorker worker(resampler);
  // The output .tsf inherits the output tractogram's (input-derived) properties
  //   so that its timestamp matches the freshly-written output .tck.
  CoupledReceiver receiver(argument[1], properties, tsf_out);
  Thread::run_ordered_queue(
      loader, Thread::batch(CoupledItem()), Thread::multi(worker), Thread::batch(CoupledItem()), receiver);
}
