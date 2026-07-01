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
#include "dwi/tractography/nonfinite.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/resampling/arc.h"
#include "dwi/tractography/resampling/downsampler.h"
#include "dwi/tractography/resampling/endpoints.h"
#include "dwi/tractography/resampling/fixed_num_points.h"
#include "dwi/tractography/resampling/fixed_step_size.h"
#include "dwi/tractography/resampling/resampling.h"
#include "dwi/tractography/resampling/upsampler.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram.h"
#include "dwi/tractography/tractogram_item.h"
#include "dwi/tractography/weights.h"
#include "image.h"
#include "math/math.h"
#include "ordered_thread_queue.h"
#include "thread.h"

#include <filesystem>
#include <memory>
#include <optional>

using namespace MR;
using namespace App;
using namespace DWI::Tractography;

// Disambiguate from the image subsystem's MR::Formats, also in scope here.
namespace TrackFormats = DWI::Tractography::Formats;

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
using item_type = TractogramItem<value_type>;

//! \brief poll the output format and reject a streamline it cannot represent.
/*! Interpolating resampling modes can extrapolate to non-finite vertex
 * positions; if the selected output format cannot represent them (e.g. ".tck",
 * which uses non-finite values as in-band delimiters) this throws a clear,
 * resampling-specific error rather than letting the writer corrupt the stream. */
void verify_output_representable(const Streamline<value_type> &tck,
                                 const TrackFormats::NonFinite output_tolerance,
                                 const std::string_view output_format) {
  const NonFiniteContent content = scan_vertices(tck);
  if (!nonfinite_permitted(output_tolerance, content))
    throw Exception("resampling produced non-finite vertex positions in streamline " + str(tck.get_index()) +
                    ", which the output tractography format (\"" + std::string(output_format) + "\") cannot represent");
}

//! \brief Queue worker: resamples a streamline and carries its sidecar data.
/*! Per-streamline (dps) fields and the streamline weight pass through unchanged
 * in every mode. Per-vertex (dpv) fields are carried only by the
 * vertex-subset-preserving modes (-downsample / -endpoints), where each field
 * is sub-sampled to exactly the retained vertices (select_dpv_vertices). The
 * interpolating modes invent new vertex positions and so carry no dpv; for those
 * modes run() has already suppressed the dpv input and -tsf_out, so in.dpv is
 * empty here. */
class Worker {
public:
  Worker(const std::unique_ptr<Resampling::Base> &in)
      : resampler(in->clone()), preserves_subset(resampler->preserves_vertex_subset()) {}

  Worker(const Worker &that) : resampler(that.resampler->clone()), preserves_subset(that.preserves_subset) {}

  bool operator()(const item_type &in, item_type &out) const {
    out.clear();
    (*resampler)(in.streamline, out.streamline);
    out.streamline.set_index(in.streamline.get_index());
    out.streamline.weight = in.streamline.weight;
    out.dps = in.dps;
    out.groups = in.groups;
    if (preserves_subset && !in.dpv.empty()) {
      out.dpv = in.dpv;
      select_dpv_vertices(out, resampler->retained_indices(in.streamline));
    }
    return true;
  }

private:
  std::unique_ptr<Resampling::Base> resampler;
  bool preserves_subset;
};

//! \brief Queue sink: rejects unrepresentable vertices, writes the item, advances progress.
class Sink {
public:
  Sink(Tractogram<value_type> &output)
      : output(output),
        output_tolerance(output.capabilities().vertices),
        output_format(output.format()),
        count(0),
        progress("resampling streamlines") {}

  bool operator()(const item_type &item) {
    verify_output_representable(item.streamline, output_tolerance, output_format);
    output.write(item);
    ++count;
    progress.set_text("resampling streamlines (count: " + str(count) + ")");
    return true;
  }

private:
  Tractogram<value_type> &output;
  const TrackFormats::NonFinite output_tolerance;
  const std::string output_format;
  size_t count;
  ProgressBar progress;
};

void run() {
  const std::unique_ptr<Resampling::Base> resampler(Resampling::get_resampler());
  const bool preserves_subset = resampler->preserves_vertex_subset();

  auto tsf_in = get_optional<std::filesystem::path>("tsf_in");
  auto tsf_out = get_optional<std::filesystem::path>("tsf_out");
  if (!tsf_in.has_value() && tsf_out.has_value())
    throw Exception("The -tsf_out option requires the -tsf_in option");

  // The interpolating modes invent new vertex positions, so per-vertex (dpv)
  //   sidecar data cannot meaningfully be carried (D8 drop): warn, suppress
  //   -tsf_out, and do not register the input .tsf for pass-through at all.
  if (tsf_in.has_value() && !preserves_subset) {
    WARN("Resampling mode interpolates new vertex positions;"
         " per-vertex (data-per-vertex) sidecar data from \"" +
         tsf_in->string() + "\"" + " cannot be carried to the output and will be dropped");
    if (tsf_out.has_value()) {
      WARN("The -tsf_out option is ignored for interpolating resampling modes");
      tsf_out = std::nullopt;
    }
    tsf_in = std::nullopt;
  }

  Properties properties;
  auto input = Tractogram<value_type>::open(argument[0], properties);
  // Route the explicitly-specified streamline weights (external file or named field
  //   of the input tractogram) into Streamline::weight; weights pass through unchanged.
  const WeightInput weight_input = register_weight_input(input, argument[0]);
  // Inject the input .tsf as a per-vertex (dpv) field so it flows through the
  //   item pipeline alongside the vertices (and is sub-sampled in lock-step).
  if (tsf_in.has_value())
    input.register_input_sidecar(tsf_in->string(), properties);

  // Declare the output field set from the (possibly sidecar-augmented) input
  //   registry, so a sidecar-aware output handler serialises it; a vertices-only
  //   format (e.g. ".tck") writes the per-vertex data to the -tsf_out sidecar.
  //   plan_weight_output resolves where the streamline weights go (embedded field,
  //   external file, or — by the provenance default — propagated / suppressed).
  const WeightOutput weight_output = plan_weight_output(input.fields(), weight_input, argument[1], properties);
  auto output = Tractogram<value_type>::create(argument[1], properties, weight_output.registry);
  apply_weight_output(output, weight_output);
  if (tsf_out.has_value())
    output.register_output_sidecar(tsf_out->string(), properties);

  {
    Worker worker(resampler);
    Sink sink(output);
    Thread::run_ordered_queue(
        input, Thread::batch(item_type(), 1024), Thread::multi(worker), Thread::batch(item_type(), 1024), sink);
  }
  output.finalise_sidecars();
}
