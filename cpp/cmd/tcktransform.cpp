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
#include "dwi/tractography/properties.h"
#include "dwi/tractography/trx_utils.h"
#include "image.h"
#include "interp/linear.h"
#include "ordered_thread_queue.h"
#include "progressbar.h"

using namespace MR;
using namespace MR::DWI;
using namespace MR::DWI::Tractography::TRX;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Apply a spatial transformation to a tracks file";

  ARGUMENTS
  + Argument ("tracks", "the input track file.").type_tracks_in().type_directory_in()
  + Argument ("transform", "the image containing the transform.").type_image_in()
  + Argument ("output", "the output track file").type_tracks_out();

}
// clang-format on

using value_type = float;
using TrackType = Tractography::Streamline<value_type>;

class Loader {
public:
  Loader(std::string_view file) : reader(open_tractogram(file, properties)) {}

  bool operator()(TrackType &item) { return (*reader)(item); }

  Tractography::Properties properties;

protected:
  std::unique_ptr<Tractography::ReaderInterface<value_type>> reader;
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

  // Public so that run() can call it directly for the TRX in-place warp path.
  Eigen::Matrix<value_type, 3, 1> pos(const Eigen::Matrix<value_type, 3, 1> &x) {
    Eigen::Matrix<value_type, 3, 1> p;
    if (interp.scanner(x)) {
      interp.index(3) = 0;
      p[0] = interp.value();
      interp.index(3) = 1;
      p[1] = interp.value();
      interp.index(3) = 2;
      p[2] = interp.value();
    } else {
      p.setConstant(std::numeric_limits<value_type>::quiet_NaN());
    }
    return p;
  }

protected:
  Interp::Linear<Image<value_type>> interp;
};

// Writer supports both TCK (Tractography::Writer) and TRX (TrxStream) output.
// For TRX output the TrxStream is finalised in the destructor.
class Writer {
public:
  Writer(std::string_view file, const Tractography::Properties &properties)
      : progress("applying spatial transformation to tracks",
                 properties.find("count") == properties.end() ? 0 : to<size_t>(properties.find("count")->second)),
        output_path(file) {
    if (is_trx(file)) {
      trx_stream = std::make_unique<trx::TrxStream>("float32");
    } else {
      tck_writer = std::make_unique<Tractography::Writer<value_type>>(file, properties);
    }
  }

  ~Writer() {
    if (trx_stream) {
      try {
        trx_stream->finalize(output_path, trx::TrxSaveOptions{});
      } catch (const std::exception &e) {
        Exception(e.what()).display();
        App::exit_error_code = 1;
      }
    }
  }

  bool operator()(const TrackType &item) {
    if (trx_stream) {
      std::vector<std::array<float, 3>> pts(item.size());
      for (size_t i = 0; i < item.size(); ++i)
        pts[i] = {item[i][0], item[i][1], item[i][2]};
      trx_stream->push_streamline(pts);
    } else {
      (*tck_writer)(item);
    }
    ++progress;
    return true;
  }

protected:
  ProgressBar progress;
  std::string output_path;
  std::unique_ptr<trx::TrxStream> trx_stream;
  std::unique_ptr<Tractography::Writer<value_type>> tck_writer;
};

void run() {
  auto data = Image<value_type>::open(argument[1]).with_direct_io(3);
  Warper warper(data);

  // TRX→TRX: warp positions directly on the loaded TrxFile.  All metadata
  // (dps, dpv, groups, dpg) is preserved automatically because we modify the
  // in-memory position array and then call save().  Vertices that fall outside
  // the warp field are left at their original coordinates with a warning.
  if (is_trx(argument[0]) && is_trx(argument[2])) {
    auto trx = load_trx(argument[0]);
    const Eigen::Index nb_s = static_cast<Eigen::Index>(trx->streamlines->_offsets.size() - 1);
    size_t vertex_drops = 0;
    ProgressBar progress("applying spatial transformation to tracks", static_cast<size_t>(nb_s));
    for (Eigen::Index s = 0; s < nb_s; ++s) {
      const Eigen::Index v0 = trx->streamlines->_offsets(s, 0);
      const Eigen::Index v1 = trx->streamlines->_offsets(s + 1, 0);
      for (Eigen::Index v = v0; v < v1; ++v) {
        const Eigen::Matrix<value_type, 3, 1> p{
            trx->streamlines->_data(v, 0), trx->streamlines->_data(v, 1), trx->streamlines->_data(v, 2)};
        const auto wp = warper.pos(p);
        if (wp.allFinite()) {
          trx->streamlines->_data(v, 0) = wp[0];
          trx->streamlines->_data(v, 1) = wp[1];
          trx->streamlines->_data(v, 2) = wp[2];
        } else {
          ++vertex_drops;
        }
      }
      ++progress;
    }
    if (vertex_drops)
      WARN(str(vertex_drops) +
           " streamline vertices fell outside the warp field and were left at "
           "their original coordinates; check that the warp field covers the full tractogram extent");
    trx->save(argument[2]);
    return;
  }

  // Stream path: handles TCK→TCK, TCK→TRX, and TRX→TCK.
  // For TRX input, open_tractogram returns a TRXReader that provides geometry
  // transparently.  For TRX output, Writer uses TrxStream (geometry only —
  // no metadata is carried over from a TCK input since there is none).
  Loader loader(argument[0]);
  Writer writer(argument[2], loader.properties);

  Thread::run_ordered_queue(
      loader, Thread::batch(TrackType(), 1024), Thread::multi(warper), Thread::batch(TrackType(), 1024), writer);
}
