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

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "perform various editing operations on track files.";

  ARGUMENTS
  + Argument ("tracks_in",  "the input track file(s)").type_tracks_in().allow_multiple()
  + Argument ("tracks_out", "the output track file").type_tracks_out();

  OPTIONS
  + ROIOption
  + LengthOption
  + TruncateOption
  + WeightsOption

  + OptionGroup ("Other options specific to tckedit")
  + Option ("inverse", "output the inverse selection of streamlines based on the criteria provided, "
                       "i.e. only those streamlines that fail at least one criterion will be written to file.")

  + Option ("ends_only", "only test the ends of each streamline against the provided include/exclude ROIs")

  // TODO Input weights with multiple input files currently not supported
  + OptionGroup ("Options for handling streamline weights")
  + Tractography::TrackWeightsInOption
  + Tractography::TrackWeightsOutOption;

  // TODO Additional options?
  // - Peak curvature threshold
  // - Mean curvature threshold

}


void erase_if_present (Tractography::Properties& p, const std::string s)
{
  auto i = p.find (s);
  if (i != p.end())
    p.erase (i);
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
  vector<std::string> input_file_list;

  for (size_t file_index = 0; file_index != num_inputs; ++file_index) {

    input_file_list.push_back (argument[file_index]);

    Properties p;
    Reader<float> reader (argument[file_index], p);

    for (vector<std::string>::const_iterator i = p.comments.begin(); i != p.comments.end(); ++i) {
      bool present = false;
      for (vector<std::string>::const_iterator j = properties.comments.begin(); !present && j != properties.comments.end(); ++j)
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
  // Due to the potential use of masking, we have no choice but to clear the
  //   properties class of any fields that would otherwise propagate through
  //   and be applied as part of this editing
  erase_if_present (properties, "min_dist");
  erase_if_present (properties, "max_dist");
  erase_if_present (properties, "min_weight");
  erase_if_present (properties, "max_weight");
  Editing::load_properties (properties);

  // Parameters that the worker threads need to be aware of, but do not appear in Properties
  const bool inverse   = get_options ("inverse").size();
  const bool ends_only = get_options ("ends_only").size();

  // Parameters that the output thread needs to be aware of
  const size_t number = get_option_value ("number", size_t(0));
  const size_t skip   = get_option_value ("skip",   size_t(0));

  Loader loader (input_file_list);
  Worker worker (properties, inverse, ends_only);
  // This needs to be run AFTER creation of the Worker class
  // (worker needs to be able to set max & min number of points based on step size in input file,
  //  receiver needs "output_step_size" field to have been updated before file creation)
  Receiver receiver (output_path, properties, number, skip);

  Thread::run_queue (
      loader, 
      Thread::batch (Streamline<>()),
      Thread::multi (worker), 
      Thread::batch (Streamline<>()),
      receiver);

}
