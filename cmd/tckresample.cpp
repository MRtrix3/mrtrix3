/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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
#include "math/math.h"
#include "image.h"
#include "ordered_thread_queue.h"
#include "thread.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/resampling/arc.h"
#include "dwi/tractography/resampling/downsampler.h"
#include "dwi/tractography/resampling/endpoints.h"
#include "dwi/tractography/resampling/fixed_num_points.h"
#include "dwi/tractography/resampling/fixed_step_size.h"
#include "dwi/tractography/resampling/resampling.h"
#include "dwi/tractography/resampling/upsampler.h"




using namespace MR;
using namespace App;
using namespace DWI::Tractography;


void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Resample each streamline in a track file to a new set of vertices";

  DESCRIPTION
  + "It is necessary to specify precisely ONE of the command-line options for "
    "controlling how this resampling takes place; this may be either increasing "
    "or decreasing the number of samples along each streamline, or may involve "
    "changing the positions of the samples according to some specified trajectory."

  + "Note that because the length of a streamline is calculated based on the sums of "
    "distances between adjacent vertices, resampling a streamline to a new set of "
    "vertices will typically change the quantified length of that streamline; the "
    "magnitude of the difference will typically depend on the discrepancy in the "
    "number of vertices, with less vertices leading to a shorter length (due to "
    "taking chordal lengths of curved trajectories).";

  ARGUMENTS
  + Argument ("in_tracks",  "the input track file").type_tracks_in()
  + Argument ("out_tracks", "the output resampled tracks").type_tracks_out();

  OPTIONS
    + Resampling::ResampleOption;

    // TODO Resample according to an exemplar streamline
}



using value_type = float;



class Worker
{ NOMEMALIGN
  public:
    Worker (const std::unique_ptr<Resampling::Base>& in) :
        resampler (in->clone()) { }

    Worker (const Worker& that) :
        resampler (that.resampler->clone()) { }

    bool operator() (const Streamline<value_type>& in, Streamline<value_type>& out) const {
      (*resampler) (in, out);
      return true;
    }

  private:
    std::unique_ptr<Resampling::Base> resampler;
};



class Receiver
{ NOMEMALIGN
  public:
    Receiver (const std::string& path, const Properties& properties) :
        writer (path, properties),
        progress ("resampling streamlines") { }

    bool operator() (const Streamline<value_type>& tck) {
      auto progress_message = [&](){ return "resampling streamlines (count: " + str(writer.count) + ", skipped: " + str(writer.total_count - writer.count) + ")"; };
      writer (tck);
      progress.set_text (progress_message());
      return true;
    }

  private:
    Writer<value_type> writer;
    ProgressBar progress;

};



void run ()
{
  Properties properties;
  Reader<value_type> read (argument[0], properties);

  const std::unique_ptr<Resampling::Base> resampler (Resampling::get_resampler());

  Worker worker (resampler);
  Receiver receiver (argument[1], properties);
  Thread::run_ordered_queue (read,
                             Thread::batch (Streamline<value_type>()),
                             Thread::multi (worker),
                             Thread::batch (Streamline<value_type>()),
                             receiver);

}
