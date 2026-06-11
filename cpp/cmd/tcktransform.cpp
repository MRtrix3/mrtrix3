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
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/weights.h"
#include "image.h"
#include "interp/linear.h"
#include "ordered_thread_queue.h"
#include "progressbar.h"
#include "registration/warp/validate.h"

#include <filesystem>
#include <optional>

using namespace MR;
using namespace MR::DWI;
using namespace App;

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
using TrackType = Tractography::Streamline<value_type>;

class Loader {
public:
  Loader(const std::filesystem::path &path) : reader(path, properties) {}

  bool operator()(TrackType &item) { return reader(item); }

  Tractography::Properties properties;

protected:
  Tractography::Reader<value_type> reader;
};

class Warper {
public:
  Warper(const Image<value_type> &warp) : interp(warp) {}

  bool operator()(const TrackType &in, TrackType &out) {
    out.clear();
    out.set_index(in.get_index());
    out.weight = in.weight;
    for (size_t n = 0; n < in.size(); ++n) {
      auto vertex = pos(in[n]);
      if (vertex.allFinite())
        out.push_back(vertex);
    }
    return true;
  }

  Eigen::Matrix<value_type, 3, 1> pos(const Eigen::Matrix<value_type, 3, 1> &x) {
    Eigen::Matrix<value_type, 3, 1> p;
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

protected:
  Interp::Linear<Image<value_type>> interp;
};

class Writer {
public:
  Writer(const std::filesystem::path &path, const Tractography::Properties &properties)
      : progress("applying spatial transformation to tracks",
                 properties.find("count") == properties.end() ? 0 : to<size_t>(properties.find("count")->second)),
        writer(path, properties) {}

  bool operator()(const TrackType &item) {
    writer(item);
    ++progress;
    return true;
  }

protected:
  ProgressBar progress;
  Tractography::Properties properties;
  Tractography::Writer<value_type> writer;
};

// ---------------------------------------------------------------------------
// Coupled (per-vertex sidecar) pipeline: -tsf_in / -tsf_out.
// A spatial transform preserves vertex order, so the per-vertex scalar is
// carried through unchanged; the sole exception is a vertex that falls outside
// the warp field and is therefore dropped, in which case its matching scalar is
// dropped too so that the two stay aligned.
// ---------------------------------------------------------------------------

using ScalarType = Tractography::TrackScalar<value_type>;

struct CoupledItem {
  TrackType streamline;
  ScalarType scalar;
  void clear() {
    streamline.clear();
    scalar.clear();
  }
};

class CoupledLoader {
public:
  CoupledLoader(const std::filesystem::path &tracks_path, const std::filesystem::path &tsf_path)
      : reader(tracks_path, properties), tsf_reader(tsf_path, tsf_properties) {}

  bool operator()(CoupledItem &item) {
    item.clear();
    if (!reader(item.streamline))
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

  Tractography::Properties properties;
  Tractography::Properties tsf_properties;

protected:
  Tractography::Reader<value_type> reader;
  Tractography::ScalarReader<value_type> tsf_reader;
};

class CoupledWarper {
public:
  CoupledWarper(const Image<value_type> &warp) : warper(warp) {}

  bool operator()(const CoupledItem &in, CoupledItem &out) {
    out.clear();
    out.streamline.set_index(in.streamline.get_index());
    out.streamline.weight = in.streamline.weight;
    out.scalar.set_index(in.streamline.get_index());
    for (size_t n = 0; n < in.streamline.size(); ++n) {
      auto vertex = warper.pos(in.streamline[n]);
      if (vertex.allFinite()) {
        out.streamline.push_back(vertex);
        // Carry the matching per-vertex scalar (identity); drop it only when the
        //   vertex itself was dropped, keeping vertices and scalars aligned.
        out.scalar.push_back(in.scalar[n]);
      }
    }
    return true;
  }

protected:
  Warper warper;
};

class CoupledWriter {
public:
  CoupledWriter(const std::filesystem::path &tracks_path,
                const Tractography::Properties &properties,
                const std::optional<std::filesystem::path> &tsf_path)
      : progress("applying spatial transformation to tracks",
                 properties.find("count") == properties.end() ? 0 : to<size_t>(properties.find("count")->second)),
        writer(tracks_path, properties) {
    if (tsf_path.has_value())
      tsf_writer.reset(new Tractography::ScalarWriter<value_type>(*tsf_path, properties));
  }

  bool operator()(const CoupledItem &item) {
    writer(item.streamline);
    if (tsf_writer)
      (*tsf_writer)(item.scalar);
    ++progress;
    return true;
  }

protected:
  ProgressBar progress;
  Tractography::Writer<value_type> writer;
  std::unique_ptr<Tractography::ScalarWriter<value_type>> tsf_writer;
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

  if (!tsf_in.has_value()) {
    if (tsf_out.has_value())
      throw Exception("The -tsf_out option requires the -tsf_in option");
    // Original (vertices-only) pipeline; per-streamline weights still pass
    //   through via Streamline::weight (the .tck handler reads/writes the
    //   -tck_weights_in/out sidecar).
    Loader loader(argument[0]);
    Warper warper(data);
    Writer writer(argument[2], loader.properties);
    Thread::run_ordered_queue(
        loader, Thread::batch(TrackType(), 1024), Thread::multi(warper), Thread::batch(TrackType(), 1024), writer);
    return;
  }

  // Coupled per-vertex sidecar pass-through pipeline.
  CoupledLoader loader(argument[0], *tsf_in);
  CoupledWarper warper(data);
  CoupledWriter writer(argument[2], loader.properties, tsf_out);
  Thread::run_ordered_queue(
      loader, Thread::batch(CoupledItem(), 1024), Thread::multi(warper), Thread::batch(CoupledItem(), 1024), writer);
}
