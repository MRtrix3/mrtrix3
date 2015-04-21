/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2012.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/


#include <vector>

#include "command.h"
#include "point.h"
#include "progressbar.h"
#include "memory.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/weights.h"
#include "dwi/tractography/connectomics/connectomics.h"
#include "dwi/tractography/connectomics/multithread.h"
#include "dwi/tractography/connectomics/tck2nodes.h"
#include "dwi/tractography/mapping/loader.h"

#include "math/matrix.h"

#include "image/buffer.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/transform.h"
#include "image/voxel.h"

#include "thread_queue.h"






using namespace MR;
using namespace App;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Connectomics;




void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "extract streamlines from a tractogram based on their connectivity to parcellated nodes.\n "
  + "By default, this command will create one track file for every edge in the connectome; "
  + "see available command-line options for altering this behaviour.";

  ARGUMENTS
  + Argument ("tracks_in",      "the input track file").type_file_in()
  + Argument ("nodes_image_in", "the input parcellated anatomical image").type_image_in()
  + Argument ("prefix_out",     "the output track file prefix").type_text();


  OPTIONS

  + Option ("nodes_between", "output track files only for connections between a particular set of nodes of interest; "
                             "only connections where both nodes appear in this list will be output to file")
    + Argument ("list").type_sequence_int()

  + Option ("nodes_to_any", "output track files only for connections involving a particular set of nodes of interest; "
                            "any connections where one of the nodes appears on this list will be output to file")
    + Argument ("list").type_sequence_int()

  + Option ("per_node", "output one track file containing the streamlines connecting each node, rather than one for each edge")

  + Option ("keep_unassigned", "by default, the program discards those streamlines that are not successfully assigned to a node pair. "
                               "Set this option to generate output files containing these streamlines (labelled as node index 0)")

  + Connectomics::AssignmentOption

  + Tractography::TrackWeightsInOption

  + Option ("prefix_tck_weights_out", "provide a prefix for outputting a text file corresponding to each output file, "
                                      "each containing only the streamline weights relevant for that track file")
    + Argument ("prefix").type_text();




};



void run ()
{

  Tractography::Properties properties;
  Tractography::Reader<float> reader (argument[0], properties);

  Image::Buffer<node_t> nodes_data (argument[1]);
  auto nodes = nodes_data.voxel();

  const std::string prefix (argument[2]);

  std::unique_ptr<Connectomics::Tck2nodes_base> tck2nodes (Connectomics::load_assignment_mode (nodes_data));

  node_t max_node_index = 0;
  Image::LoopInOrder loop (nodes);
  for (auto l = loop (nodes); l; ++l)
    max_node_index = MAX (max_node_index, nodes.value());

  INFO ("Maximum node index is " + str(max_node_index));

  const node_t first_node = get_options ("keep_unassigned").size() ? 0 : 1;

  NodeExtractMapper mapper (*tck2nodes);
  NodeExtractWriter writer (properties);

  Options opt = get_options ("prefix_tck_weights_out");
  const std::string weights_prefix = opt.size() ? std::string (opt[0][0]) : "";

  if (get_options ("per_node").size()) {

    if (get_options ("nodes_between").size())
      throw Exception ("Options -per_node and -nodes_between cannot currently be used together");

    Options opt = get_options ("nodes_to_any");
    if (opt.size()) {
      std::vector<int> node_list = opt[0][0];
      for (std::vector<int>::const_iterator i = node_list.begin(); i != node_list.end(); ++i) {
        if (node_t(*i) > max_node_index)
          throw Exception ("Node index " + str(*i) + " exceeds the maximum index in the parcellation image (" + str(max_node_index) + ")");
        writer.add (node_t(*i), prefix + str(*i) + ".tck", weights_prefix.size() ? (weights_prefix + "_" + str(*i) + ".csv") : "");
      }
    } else {
      for (node_t i = first_node; i <= max_node_index; ++i)
        writer.add (i, prefix + str(i) + ".tck", weights_prefix.size() ? (weights_prefix + "_" + str(i) + ".csv") : "");
    }

  } else {

    Math::Matrix<float> outputs (max_node_index + 1, max_node_index + 1);
    outputs.zero();

    const Options opt_between = get_options ("nodes_between");
    const Options opt_to_any  = get_options ("nodes_to_any");
    if (opt_between.size() && opt_to_any.size())
      throw Exception ("Options -nodes_between and -nodes_to_any cannot sensibly be used together");

    if (opt_between.size()) {
      std::vector<int> node_list = opt_between[0][0];
      for (size_t i = 0; i != node_list.size(); ++i) {
        for (size_t j = i + 1; j != node_list.size(); ++j)
          outputs (node_list[i], node_list[j]) = outputs (node_list[j], node_list[i]) = 1.0;
      }
    }
    if (opt_to_any.size()) {
      std::vector<int> node_list = opt_to_any[0][0];
      for (std::vector<int>::const_iterator i = node_list.begin(); i != node_list.end(); ++i) {
        for (size_t j = 0; j <= max_node_index; ++j)
          outputs (*i, j) = outputs (j, *i) = 1.0;
      }
    }
    if (!opt_between.size() && !opt_to_any.size()) {
      outputs = 1.0;
      if (first_node) {
        outputs.row(0) = 0;
        outputs.column(0) = 0;
      }
    }

    for (size_t i = 0; i <= max_node_index; ++i) {
      for (size_t j = i; j <= max_node_index; ++j) {
        if (outputs (i, j))
          writer.add (i, j, prefix + str(i) + "-" + str(j) + ".tck", weights_prefix.size() ? (weights_prefix + "_" + str(i) + "_" + str(j) + ".csv") : "");
      }
    }

  }

  INFO ("A total of " + str (writer.file_count()) + " output track files will be generated");

  Mapping::TrackLoader loader (reader, properties["count"].empty() ? 0 : to<size_t>(properties["count"]), "extracting streamlines of interest... ");
  Thread::run_queue (
      loader, 
      Thread::batch (Tractography::Streamline<float>()), 
      Thread::multi (mapper), 
      Thread::batch (MappedTrackWithData()), 
      writer);

}
