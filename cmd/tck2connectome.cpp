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
#include <set>

#include "command.h"
#include "ptr.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/weights.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/connectomics/connectomics.h"
#include "dwi/tractography/connectomics/edge_metrics.h"
#include "dwi/tractography/connectomics/multithread.h"
#include "dwi/tractography/connectomics/tck2nodes.h"


#include "image/buffer.h"
#include "image/loop.h"
#include "image/voxel.h"

#include "thread/queue.h"





using namespace MR;
using namespace App;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Connectomics;



void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "generate a connectome matrix from a streamlines file and a node parcellation image";

  ARGUMENTS
  + Argument ("tracks_in",      "the input track file").type_file()
  + Argument ("nodes_in",       "the input node parcellation image").type_image_in()
  + Argument ("connectome_out", "the output .csv file containing edge weights").type_file();


  OPTIONS
  + Connectomics::AssignmentOption
  + Connectomics::MetricOption

  + Tractography::TrackWeightsInOption

  + Option ("keep_unassigned", "By default, the program discards the information regarding those streamlines that are not successfully assigned to a node pair. "
                               "Set this option to keep these values (will be the first row/column in the output matrix)")

  + Option ("zero_diagonal", "set all diagonal entries in the matrix to zero \n"
                             "(these represent streamlines that connect to the same node at both ends)");


};



void run ()
{

  Tractography::Properties properties;
  Tractography::Reader<float> reader (argument[0], properties);

  Image::Buffer<node_t> nodes_data (argument[1]);
  Image::Buffer<node_t>::voxel_type nodes (nodes_data);

  Ptr<Connectomics::Metric_base>    metric    (Connectomics::load_metric (nodes_data));
  Ptr<Connectomics::Tck2nodes_base> tck2nodes (Connectomics::load_assignment_mode (nodes_data));

  // First, find out how many segmented nodes there are, so the matrix can be pre-allocated
  node_t max_node_index = 0;
  Image::LoopInOrder loop (nodes);
  for (loop.start (nodes); loop.ok(); loop.next (nodes)) {
    if (nodes.value() > max_node_index)
      max_node_index = nodes.value();
  }

  // Check for node volume for all nodes
  std::vector<uint32_t> node_volumes (max_node_index + 1);
  for (loop.start (nodes); loop.ok(); loop.next (nodes))
    ++node_volumes[nodes.value()];
  std::set<node_t> missing_nodes;
  for (size_t i = 1; i != node_volumes.size(); ++i) {
    if (!node_volumes[i])
      missing_nodes.insert (i);
  }
  if (missing_nodes.size()) {
    WARN ("The following nodes are missing from the parcellation image:");
    std::set<node_t>::iterator i = missing_nodes.begin();
    std::string list = str(*i);
    for (++i; i != missing_nodes.end(); ++i)
      list += ", " + str(*i);
    WARN (list);
    WARN ("(This may indicate poor parcellation image preparation, use of incorrect config file in mrprep4connectome, or very poor registration)");
  }

  // Multi-threaded connectome construction
  Mapping::TrackLoader loader (reader, properties["count"].empty() ? 0 : to<size_t>(properties["count"]), "Constructing connectome... ");
  Mapper mapper (*tck2nodes, *metric);
  Connectome connectome (max_node_index);
  Thread::run_queue (
      loader, 
      Thread::batch (Tractography::Streamline<float>()), 
      Thread::multi (mapper), 
      Thread::batch (Mapped_track()), 
      connectome);

  if (metric->scale_edges_by_streamline_count())
    connectome.scale_by_streamline_count();

  connectome.error_check (missing_nodes);

  Options opt = get_options ("keep_unassigned");
  if (!opt.size())
    connectome.remove_unassigned();

  opt = get_options ("zero_diagonal");
  if (opt.size())
    connectome.zero_diagonal();

  connectome.write (argument[2]);

}
