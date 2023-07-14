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

#include <string>

#include "command.h"
#include "exception.h"
#include "mrtrix.h"
#include "ordered_thread_queue.h"
#include "types.h"

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

  SYNOPSIS = "Perform various editing operations on track files";

  DESCRIPTION
  + "This command can be used to perform various types of manipulations "
    "on track data. A range of such manipulations are demonstrated in the "
    "examples provided below.";

  EXAMPLES
  + Example ("Concatenate data from multiple track files into one",
             "tckedit *.tck all_tracks.tck",
             "Here the wildcard operator is used to select all files in the "
             "current working directory that have the .tck filetype suffix; but "
             "input files can equivalently be specified one at a time explicitly.")

  + Example ("Extract a reduced number of streamlines",
             "tckedit in_many.tck out_few.tck -number 1k -skip 500",
             "The number of streamlines requested would typically be less "
             "than the number of streamlines in the input track file(s); if it "
             "is instead greater, then the command will issue a warning upon "
             "completion. By default the streamlines for the output file are "
             "extracted from the start of the input file(s); in this example the "
             "command is instead instructed to skip the first 500 streamlines, and "
             "write to the output file streamlines 501-1500.")

  + Example ("Extract streamlines based on selection criteria",
             "tckedit in.tck out.tck -include ROI1.mif -include ROI2.mif -minlength 25",
             "Multiple criteria can be added in a single invocation of tckedit, "
             "and a streamline must satisfy all criteria imposed in order to be "
             "written to the output file. Note that both -include and -exclude "
             "options can be specified multiple times to provide multiple "
             "waypoints / exclusion masks.")

  + Example ("Select only those streamline vertices within a mask",
             "tckedit in.tck cropped.tck -mask mask.mif",
             "The -mask option is applied to each streamline vertex independently, "
             "rather than to each streamline, retaining only those streamline vertices "
             "within the mask. As such, use of this option may result in a greater "
             "number of output streamlines than input streamlines, as a single input "
             "streamline may have the vertices at either endpoint retained but some "
             "vertices at its midpoint removed, effectively cutting one long streamline "
             "into multiple shorter streamlines.");



  ARGUMENTS
  + Argument ("tracks_in",  "the input track file(s)").type_tracks_in().allow_multiple()
  + Argument ("tracks_out", "the output track file").type_tracks_out();

  OPTIONS
  + ROIOption
  + LengthOption
  + TruncateOption
  + WeightsOption

  + OptionGroup ("Other options specific to tckedit")
  + Option ("inverse", "output the inverse selection of streamlines based on the criteria provided; "
                       "i.e. only those streamlines that fail at least one selection criterion, "
                       "and/or vertices that are outside masks if provided, will be written to file")

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
    Reader<float> (argument[file_index], p);

    for (const auto& i : p.comments) {
      bool present = false;
      for (const auto& j: properties.comments)
        if ( (present = (i == j)) )
          break;
      if (!present)
        properties.comments.push_back (i);
    }

    for (const auto& i : p.prior_rois) {
      const auto potential_matches = properties.prior_rois.equal_range (i.first);
      bool present = false;
      for (auto j = potential_matches.first; !present && j != potential_matches.second; ++j)
        present = (i.second == j->second);
      if (!present)
        properties.prior_rois.insert (i);
    }

    size_t this_count = 0;

    for (const auto& i : p) {
      if (i.first == "count") {
        this_count = to<float> (i.second);
      } else {
        auto existing = properties.find (i.first);
        if (existing == properties.end())
          properties.insert (i);
        else if (i.second != existing->second)
          existing->second = "variable";
      }
    }

    count += this_count;

  }

  DEBUG ("estimated number of input tracks: " + str(count));

  // Remove keyval "total_count", as there is ambiguity about what _should_ be
  //   contained in that field upon editing one or more existing tractograms
  //   (it has a specific interpretation in the context of streamline generation only)
  erase_if_present (properties, "total_count");

  load_rois (properties);
  properties.compare_stepsize_rois();

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
  Receiver receiver (output_path, properties, number, skip);

  Thread::run_ordered_queue (
      loader,
      Thread::batch (Streamline<>()),
      Thread::multi (worker),
      Thread::batch (Streamline<>()),
      receiver);

}
