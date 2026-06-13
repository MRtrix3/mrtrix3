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
#include "dwi/tractography/tractogram.h"
#include "dwi/tractography/weights.h"
#include "image.h"
#include "interp/linear.h"
#include "ordered_thread_queue.h"
#include "progressbar.h"
#include "registration/warp/validate.h"

#include <atomic>
#include <filesystem>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

using namespace MR;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace App;

// Disambiguate from the image subsystem's MR::Formats, also in scope here.
namespace TrackFormats = MR::DWI::Tractography::Formats;

// clang-format off
void usage() {

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Apply a spatial transformation to a tracks file";

  DESCRIPTION
  + "Unlike the non-linear transformation of image data,"
    " where the value of the deformation field in a destination voxel position"
    " defines the location in space from which to \"pull\" image data into that voxel,"
    " the non-linear transformation of streamlines data"
    " involves sampling the deformation field at each streamline vertex location"
    " to determine the new spatial location to which to \"push\" that vertex."
    " As such, the appropriate deformation field to apply to streamlines data"
    " is the inverse of what would be applied to image data."
    " So for instance, this may involve the utilisation of a template-to-subject warp field"
    " in order to transform streamlines from subject to template space.";

  DESCRIPTION
  + "Sidecar data associated with the streamlines are passed through unchanged,"
    " since a spatial transformation relocates existing vertices without altering"
    " their number or order:"
    " per-streamline weights (-tck_weights_in/out) and per-vertex data"
    " (a track scalar file via -tsf_in / -tsf_out) remain valid for the"
    " transformed streamlines and are carried across verbatim.";

  DESCRIPTION
  + "A streamline vertex that falls outside the field of view of the deformation field"
    " has no defined transformed location, and is assigned a non-finite (NaN) position."
    " How such vertices are handled on output depends on whether the selected output format"
    " can represent non-finite vertex coordinates:"
    " a format that cannot (e.g. \".tck\", which uses non-finite values as in-band delimiters)"
    " has those vertices culled from the output,"
    " and a warning is issued quoting the number of streamlines so affected;"
    " a format that can (e.g. \".trk\", \".trx\") retains those vertices in the output,"
    " and a warning is issued quoting the number of streamlines that contain non-finite vertex data.";

  ARGUMENTS
  + Argument ("tracks", "the input track file.").type_tracks_in()
  + Argument ("transform", "the image containing the transform.").type_image_in()
  + Argument ("output", "the output track file").type_tracks_out();

  OPTIONS
  + OptionGroup ("Options for handling sidecar data")
  + Tractography::TrackWeightsInOption
  + Tractography::TrackWeightsOutOption
  + Option ("tsf_in", "an input track scalar file (.tsf) of per-vertex data,"
                      " passed through unchanged to correspond to the transformed vertices")
    + Argument ("path").type_file_in()
  + Option ("tsf_out", "the output track scalar file (.tsf) corresponding to -tsf_in")
    + Argument ("path").type_file_out();

}
// clang-format on

using value_type = float;
using item_type = TractogramItem<value_type>;

//! \brief Samples the deformation field at each streamline vertex and applies
//!   the output-format-dependent non-finite vertex policy.
/*! A vertex that falls outside the deformation field is assigned a non-finite
 * (NaN) position. If the selected output format cannot represent non-finite
 * vertices (TrackFormats::NonFinite::Forbidden) such vertices are culled — together
 * with their matching per-vertex (dpv) sidecar rows, so the two stay aligned —
 * and any streamline that lost a vertex is tallied. Otherwise the vertices are
 * retained, and any streamline that contains non-finite vertex data is tallied.
 * The tally is a shared atomic so the count survives the multi-threaded fan-out
 * (the functor is copied per worker; the counter is shared). */
class Warper {
public:
  Warper(const Image<value_type> &warp,
         const TrackFormats::NonFinite vertex_tolerance,
         const std::shared_ptr<std::atomic<size_t>> &affected)
      : interp(warp), vertex_tolerance(vertex_tolerance), affected(affected) {}

  bool operator()(const item_type &in, item_type &out) {
    out.clear();
    out.streamline.set_index(in.streamline.get_index());
    out.streamline.weight = in.streamline.weight;
    out.dps = in.dps;
    out.dpv = in.dpv;
    out.groups = in.groups;

    if (vertex_tolerance == TrackFormats::NonFinite::Forbidden) {
      // Cull non-finite vertices, sub-sampling the per-vertex sidecar to match.
      std::vector<size_t> kept;
      kept.reserve(in.streamline.size());
      for (size_t n = 0; n != in.streamline.size(); ++n) {
        const Eigen::Matrix<value_type, 3, 1> vertex = pos(in.streamline[n]);
        if (vertex.allFinite()) {
          out.streamline.push_back(vertex);
          kept.push_back(n);
        }
      }
      if (kept.size() != in.streamline.size()) {
        ++(*affected);
        select_dpv_vertices(out, kept);
      }
    } else {
      // Retain every vertex, including the non-finite ones the format can carry.
      bool contains_nonfinite = false;
      for (size_t n = 0; n != in.streamline.size(); ++n) {
        const Eigen::Matrix<value_type, 3, 1> vertex = pos(in.streamline[n]);
        if (!vertex.allFinite())
          contains_nonfinite = true;
        out.streamline.push_back(vertex);
      }
      if (contains_nonfinite)
        ++(*affected);
    }
    return true;
  }

protected:
  Interp::Linear<Image<value_type>> interp;
  TrackFormats::NonFinite vertex_tolerance;
  std::shared_ptr<std::atomic<size_t>> affected;

  //! \brief the transformed position of \a x, or a NaN vector if outside the field.
  Eigen::Matrix<value_type, 3, 1> pos(const Eigen::Matrix<value_type, 3, 1> &x) {
    Eigen::Matrix<value_type, 3, 1> p =
        Eigen::Matrix<value_type, 3, 1>::Constant(std::numeric_limits<value_type>::quiet_NaN());
    if (interp.scanner(x)) {
      interp.index(3) = 0;
      p[0] = interp.value();
      interp.index(3) = 1;
      p[1] = interp.value();
      interp.index(3) = 2;
      p[2] = interp.value();
    }
    return p;
  }
};

//! \brief Queue sink: writes the transformed item and advances the progress bar.
class Sink {
public:
  Sink(Tractogram<value_type> &output, const Properties &properties)
      : output(output),
        progress("applying spatial transformation to tracks",
                 properties.find("count") == properties.end() ? 0 : to<size_t>(properties.find("count")->second)) {}

  bool operator()(const item_type &item) {
    output.write(item);
    ++progress;
    return true;
  }

protected:
  Tractogram<value_type> &output;
  ProgressBar progress;
};

void run() {
  Header H_warp = Header::open(argument[1]);
  auto warp_format = Registration::Warp::validate_header(H_warp);
  if (warp_format != Registration::Warp::WarpFormat::Simple)
    throw Exception("Command is only compatible with 4D deformation warp fields,"
                    " not the 5D \"full\" warp format"
                    " (see eg. command \"warpconvert\")");
  auto data = H_warp.get_image<value_type>(DirectIO{3});
  Registration::Warp::debug_validate_image(data);

  auto tsf_in = get_optional<std::filesystem::path>("tsf_in");
  auto tsf_out = get_optional<std::filesystem::path>("tsf_out");
  if (!tsf_in.has_value() && tsf_out.has_value())
    throw Exception("The -tsf_out option requires the -tsf_in option");

  Properties properties;
  auto input = Tractogram<value_type>::open(argument[0], properties);
  // Inject the input .tsf as a per-vertex (dpv) field so it flows through the
  //   item pipeline alongside the vertices (and is culled in lock-step below).
  if (tsf_in.has_value())
    input.register_input_sidecar(tsf_in->string(), properties);

  // Declare the output field set from the (possibly sidecar-augmented) input
  //   registry, and reference the warp field's grid for grid-relative formats.
  auto output = Tractogram<value_type>::create(
      argument[2], properties, input.fields(), AccessRequest::Streaming, OptionalHeader(std::cref(H_warp)));
  if (tsf_out.has_value())
    output.register_output_sidecar(tsf_out->string(), properties);

  const TrackFormats::NonFinite vertex_tolerance = output.capabilities().vertices;
  auto affected = std::make_shared<std::atomic<size_t>>(0);

  {
    Warper warper(data, vertex_tolerance, affected);
    Sink sink(output, properties);
    Thread::run_ordered_queue(
        input, Thread::batch(item_type(), 1024), Thread::multi(warper), Thread::batch(item_type(), 1024), sink);
  }
  output.finalise_sidecars();

  const size_t count = affected->load();
  if (count != 0) {
    if (vertex_tolerance == TrackFormats::NonFinite::Forbidden) {
      WARN(str(count) + " streamline" + (count == 1 ? " has" : "s have") +
           " had non-finite vertices culled from the output," + " as the output format (\"" + output.format() +
           "\") cannot represent them");
    } else {
      WARN(str(count) + " streamline" + (count == 1 ? "" : "s") +
           " contain non-finite vertex data, retained in the output" + " (format \"" + output.format() + "\")");
    }
  }
}
