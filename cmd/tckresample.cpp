/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "command.h"
#include "math/math.h"
#include "image.h"
#include "thread.h"
#include "thread_queue.h"
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
  + "This may be either increasing or decreasing the number of samples along "
    "each streamline, or changing the positions of the samples according to "
    "some specified trajectory."

  + DWI::Tractography::preserve_track_order_desc;

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

  const float old_step_size = get_step_size (properties);
  if (!std::isfinite (old_step_size)) {
    INFO ("Do not have step size information from input track file");
  }

  float new_step_size = NaN;
  if (dynamic_cast<Resampling::FixedStepSize*>(resampler.get())) {
    new_step_size = dynamic_cast<Resampling::FixedStepSize*> (resampler.get())->get_step_size();
  } else if (std::isfinite (old_step_size)) {
    if (dynamic_cast<Resampling::Downsampler*>(resampler.get()))
      new_step_size = old_step_size * dynamic_cast<Resampling::Downsampler*> (resampler.get())->get_ratio();
    if (dynamic_cast<Resampling::Upsampler*>(resampler.get()))
      new_step_size = old_step_size / dynamic_cast<Resampling::Upsampler*> (resampler.get())->get_ratio();
  }
  properties["output_step_size"] = std::isfinite (new_step_size) ? str(new_step_size) : "variable";

  auto downsample = properties.find ("downsample_factor");
  if (downsample != properties.end())
    properties.erase (downsample);

  Worker worker (resampler);
  Receiver receiver (argument[1], properties);
  Thread::run_queue (read,
                     Thread::batch (Streamline<value_type>()),
                     Thread::multi (worker),
                     Thread::batch (Streamline<value_type>()),
                     receiver);

}

