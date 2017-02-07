/* Copyright (c) 2008-2017 the MRtrix3 contributors
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

  DESCRIPTION
  + "Resample each streamline to a new set of vertices. "
  + "This may be either increasing or decreasing the number of samples along "
    "each streamline, or changing the positions of the samples according to "
    "some specified trajectory.";

  ARGUMENTS
  + Argument ("in_tracks",  "the input track file").type_tracks_in()
  + Argument ("out_tracks", "the output resampled tracks").type_tracks_out();

  OPTIONS
    + Resampling::ResampleOption;

    // TODO Resample according to an exemplar streamline
}



typedef float value_type;

void run ()
{
  Properties properties;
  Reader<value_type> read (argument[0], properties);

  Resampling::Base* resampler = Resampling::get_resampler();

  float old_step_size = NaN;
  try {
    Properties::const_iterator i = properties.find ("output_step_size");
    if (i == properties.end()) {
      i = properties.find ("step_size");
      if (i != properties.end())
        old_step_size = to<float> (i->second);
    } else {
      old_step_size = to<float> (i->second);
    }
  } catch (...) {
    DEBUG ("Unable to read input track file step size");
  }

  if (std::isfinite (old_step_size)) {
    float new_step_size = NaN;
    if (typeid (*resampler) == typeid (Resampling::Downsampler))
      new_step_size = old_step_size * dynamic_cast<Resampling::Downsampler*> (resampler)->get_ratio();
    if (typeid (*resampler) == typeid (Resampling::FixedStepSize))
      new_step_size = dynamic_cast<Resampling::FixedStepSize*> (resampler)->get_step_size();
    if (typeid (*resampler) == typeid (Resampling::Upsampler))
      new_step_size = old_step_size / dynamic_cast<Resampling::Upsampler*> (resampler)->get_ratio();
    properties["output_step_size"] = std::isfinite (new_step_size) ? str(new_step_size) : "variable";
  }

  auto downsample = properties.find ("downsample_factor");
  if (downsample != properties.end())
    properties.erase (downsample);

  DWI::Tractography::Writer<value_type> writer (argument[1], properties);

  size_t skipped = 0, count = 0;
  auto progress_message = [&](){ return "sampling streamlines (count: " + str(count) + ", skipped: " + str(skipped) + ")"; };
  ProgressBar progress ("sampling streamlines");

  // Single-threaded in order to retain order of streamlines in the output file
  DWI::Tractography::Streamline<value_type> tck;
  while (read (tck)) {
    if (!resampler->limits (tck)) { skipped++; continue; }
    (*resampler) (tck);
    writer (tck);
    ++count;
    progress.update (progress_message);
  }
  progress.set_text (progress_message());

}

