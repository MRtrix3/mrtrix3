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
#include "dwi/tractography/trx_utils.h"
#include "image.h"
#include "math/math.h"
#include "ordered_thread_queue.h"
#include "thread.h"

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

  + "When the input is a TRX file and the output is also TRX,"
    " per-streamline data (dps) and groups are preserved."
    " Per-vertex data (dpv) cannot be preserved because resampling changes"
    " the number of vertices per streamline and will be discarded with a warning.";

  ARGUMENTS
  + Argument ("in_tracks", "the input track file").type_tracks_in().type_directory_in()
  + Argument ("out_tracks", "the output resampled tracks").type_tracks_out();

  OPTIONS
    + Resampling::ResampleOption;

    // TODO Resample according to an exemplar streamline
}
// clang-format on

using value_type = float;

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
  Receiver(std::string_view path, const Properties &properties)
      : output_path(path), progress("resampling streamlines"), count_(0) {
    if (TRX::is_trx(path)) {
      trx_stream = std::make_unique<trx::TrxStream>("float32");
    } else {
      tck_writer = std::make_unique<Writer<value_type>>(path, properties);
    }
  }

  ~Receiver() {
    if (trx_stream) {
      try {
        trx_stream->finalize(output_path, trx::TrxSaveOptions{});
      } catch (const std::exception &e) {
        Exception(e.what()).display();
        App::exit_error_code = 1;
      }
    }
  }

  bool operator()(const Streamline<value_type> &tck) {
    if (trx_stream) {
      std::vector<std::array<float, 3>> pts(tck.size());
      for (size_t i = 0; i < tck.size(); ++i)
        pts[i] = {tck[i][0], tck[i][1], tck[i][2]};
      trx_stream->push_streamline(pts);
      ++count_;
      progress.set_text("resampling streamlines (count: " + str(count_) + ")");
    } else {
      auto progress_message = [&]() {
        return "resampling streamlines (count: " + str(tck_writer->count) +
               ", skipped: " + str(tck_writer->total_count - tck_writer->count) + ")";
      };
      (*tck_writer)(tck);
      progress.set_text(progress_message());
    }
    return true;
  }

private:
  std::string output_path;
  ProgressBar progress;
  size_t count_;
  std::unique_ptr<trx::TrxStream> trx_stream;
  std::unique_ptr<Writer<value_type>> tck_writer;
};

void run() {
  Properties properties;
  auto reader = TRX::open_tractogram(argument[0], properties);

  const bool trx_to_trx = TRX::is_trx(argument[0]) && TRX::is_trx(argument[1]);
  if (trx_to_trx) {
    auto src = TRX::load_trx(argument[0]);
    if (src && !src->data_per_vertex.empty())
      WARN(str(src->data_per_vertex.size()) + " per-vertex data field(s) will be discarded:"
                                              " vertex count changes after resampling");
  }

  const std::unique_ptr<Resampling::Base> resampler(Resampling::get_resampler());

  Worker worker(resampler);
  {
    Receiver receiver(argument[1], properties);
    Thread::run_ordered_queue(*reader,
                              Thread::batch(Streamline<value_type>()),
                              Thread::multi(worker),
                              Thread::batch(Streamline<value_type>()),
                              receiver);
  } // receiver destructs here: TrxStream::finalize() creates the output TRX file

  if (trx_to_trx)
    TRX::copy_trx_sidecar_data(argument[0], argument[1], false);
}
