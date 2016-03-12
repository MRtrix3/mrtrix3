/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include <string>
#include <vector>

#include "command.h"
#include "exception.h"
#include "mrtrix.h"

#include "thread_queue.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/roi.h"
#include "dwi/tractography/weights.h"

#include "dwi/tractography/editing/editing.h"
#include "dwi/tractography/editing/loader.h"
#include "dwi/tractography/editing/receiver.h"
#include "dwi/tractography/editing/worker.h"



using namespace MR;
using namespace App;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Editing;




void usage ()
{

  AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "perform various editing operations on track files.";

  ARGUMENTS
  + Argument ("tracks_in",  "the input track file(s)").type_tracks_in().allow_multiple()
  + Argument ("tracks_out", "the output track file").type_tracks_out();

  OPTIONS
  + ROIOption
  + LengthOption
  + ResampleOption
  + TruncateOption
  + WeightsOption

  + Option ("inverse", "output the inverse selection of streamlines based on the criteria provided, "
                       "i.e. only those streamlines that fail at least one criterion will be written to file.")

  + Option ("test_ends_only", "only test the ends of each streamline against the provided include/exclude ROIs")

  // TODO Input weights with multiple input files currently not supported
  + Tractography::TrackWeightsInOption
  + Tractography::TrackWeightsOutOption;

  // TODO Additional options?
  // - Peak curvature threshold
  // - Mean curvature threshold
  // - Resample streamline to fixed step size



}






void update_output_step_size (Tractography::Properties& properties, const int upsample_ratio, const int downsample_ratio)
{
  if (upsample_ratio == 1 && downsample_ratio == 1)
    return;
  float step_size = 0.0;
  if (properties.find ("output_step_size") == properties.end())
    step_size = (properties.find ("step_size") == properties.end() ? 0.0 : to<float>(properties["step_size"]));
  else
    step_size = to<float>(properties["output_step_size"]);
  properties["output_step_size"] = str(step_size * float(downsample_ratio) / float(upsample_ratio));
}





void run ()
{

  const size_t num_inputs = argument.size() - 1;
  const std::string output_path = argument[num_inputs];

  // Make sure configuration is sensible
  if (get_options("tck_weights_in").size() && num_inputs > 1)
    throw Exception ("Cannot use per-streamline weighting with multiple input files");

  // Get the consensus streamline properties from among the multiple input files
  Tractography::Properties properties;
  size_t count = 0;
  std::vector<std::string> input_file_list;

  for (size_t file_index = 0; file_index != num_inputs; ++file_index) {

    input_file_list.push_back (argument[file_index]);

    Properties p;
    Reader<float> reader (argument[file_index], p);

    for (std::vector<std::string>::const_iterator i = p.comments.begin(); i != p.comments.end(); ++i) {
      bool present = false;
      for (std::vector<std::string>::const_iterator j = properties.comments.begin(); !present && j != properties.comments.end(); ++j)
        present = (*i == *j);
      if (!present)
        properties.comments.push_back (*i);
    }

    // ROI paths are ignored - otherwise tckedit will try to find the ROIs used
    //   during streamlines generation!

    size_t this_count = 0, this_total_count = 0;

    for (Properties::const_iterator i = p.begin(); i != p.end(); ++i) {
      if (i->first == "count") {
        this_count = to<float>(i->second);
      } else if (i->first == "total_count") {
        this_total_count += to<float>(i->second);
      } else {
        Properties::iterator existing = properties.find (i->first);
        if (existing == properties.end())
          properties.insert (*i);
        else if (i->second != existing->second)
          existing->second = "variable";
      }
    }

    count += this_count;

  }

  DEBUG ("estimated number of input tracks: " + str(count));

  load_rois (properties);

  // Some properties from tracking may be overwritten by this editing process
  Editing::load_properties (properties);

  // Parameters that the worker threads need to be aware of, but do not appear in Properties
  auto opt = get_options ("upsample");
  const int upsample   = opt.size() ? int(opt[0][0]) : 1;
  opt = get_options ("downsample");
  const int downsample = opt.size() ? int(opt[0][0]) : 1;
  const bool inverse = get_options ("inverse").size();
  const bool test_ends_only = get_options ("test_ends_only").size();
  const bool out_ends_only = get_options ("out_ends_only").size();

  // Parameters that the output thread needs to be aware of
  opt = get_options ("number");
  const size_t number = opt.size() ? size_t(opt[0][0]) : 0;
  opt = get_options ("skip");
  const size_t skip   = opt.size() ? size_t(opt[0][0]) : 0;

  Loader loader (input_file_list);
  Worker worker (properties, upsample, downsample, inverse, test_ends_only);
  // This needs to be run AFTER creation of the Worker class
  // (worker needs to be able to set max & min number of points based on step size in input file,
  //  receiver needs "output_step_size" field to have been updated before file creation)
  update_output_step_size (properties, upsample, downsample);
  Receiver receiver (output_path, properties, number, skip, out_ends_only);

  Thread::run_queue (
      loader, 
      Thread::batch (Streamline<>()),
      Thread::multi (worker), 
      Thread::batch (Streamline<>()),
      receiver);

}
