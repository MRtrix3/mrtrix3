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


#include "app.h"
#include "point.h"
#include "progressbar.h"
#include "ptr.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/connectomics/connectomics.h"
#include "dwi/tractography/connectomics/multithread.h"
#include "dwi/tractography/connectomics/tck2nodes.h"
#include "dwi/tractography/mapping/loader.h"

#include "image/buffer.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/transform.h"
#include "image/voxel.h"

#include "thread/queue.h"





MRTRIX_APPLICATION

using namespace MR;
using namespace App;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Connectomics;




void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "extract streamlines from a tractogram based on their connectivity to specific nodes";

  ARGUMENTS
  + Argument ("tracks_in",      "the input track file").type_file()
  + Argument ("nodes_image_in", "the input parcellated anatomical image").type_image_in();


  OPTIONS

  + Connectomics::AssignmentOption

  + Option ("node_single", "output all streamlines attributed to a particular node (regardless of the other connected node) to a single .tck file").allow_multiple()
    + Argument ("index").type_integer (0, 0, std::numeric_limits<node_t>::max())
    + Argument ("name").type_file()

  + Option ("node_pair", "output streamlines attributed to a particular node pair to a .tck file").allow_multiple()
    + Argument ("index_one").type_integer (0, 0, std::numeric_limits<node_t>::max())
    + Argument ("index_two").type_integer (0, 0, std::numeric_limits<node_t>::max())
    + Argument ("name").type_file()

  + Option ("node_all_edges", "for a node of interest, output a nomber of .tck files, where each contains the connections between some node in the parcellation, and the node of interest").allow_multiple()
    + Argument ("index").type_integer (0, 0, std::numeric_limits<node_t>::max())
    + Argument ("prefix").type_text()

  + Option ("batch_nodes", "output one .tck file for each node, where each contains all streamlines connected to that node")
    + Argument ("prefix").type_text();

};



void run ()
{

  Tractography::Properties properties;
  Tractography::Reader<float> reader (argument[0], properties);

  Image::Buffer<node_t> nodes_data (argument[1]);
  Image::Buffer<node_t>::voxel_type nodes (nodes_data);

  Ptr<Connectomics::Tck2nodes_base> tck2nodes (Connectomics::load_assignment_mode (nodes_data));

  node_t max_node_index = 0;
  Image::LoopInOrder loop (nodes);
  for (loop.start (nodes); loop.ok(); loop.next (nodes))
    max_node_index = MAX (max_node_index, nodes.value());

  NodeExtractMapper mapper (*tck2nodes);
  NodeExtractWriter writer (properties);

  Options opt = get_options ("node_single");
  for (size_t i = 0; i != opt.size(); ++i)
    writer.add (int(opt[i][0]), opt[i][1]);

  opt = get_options ("node_pair");
  for (size_t i = 0; i != opt.size(); ++i)
    writer.add (int(opt[i][0]), int(opt[i][1]), opt[i][2]);

  opt = get_options ("node_all_edges");
  for (size_t i = 0; i != opt.size(); ++i) {
    const node_t node_of_interest = size_t(opt[i][0]);
    const std::string& prefix = opt[i][1];
    for (node_t j = 0; j <= max_node_index; ++j)
      writer.add (node_of_interest, j, prefix + "_" + str(j) + ".tck");
  }

  opt = get_options ("batch_nodes");
  if (opt.size()) {
    for (size_t i = 0; i <= max_node_index; ++i)
      writer.add (i, str(opt[0][0]) + "_" + str(i) + ".tck");
  }

  if (writer.file_count()) {
    Mapping::TrackLoader loader (reader, properties["count"].empty() ? 0 : to<size_t>(properties["count"]), "extracting streamlines of interest... ");
    Thread::run_batched_queue_threaded_pipe (loader, Mapping::TrackAndIndex(), 100, mapper, MappedTrackWithData(), 100, writer);
  } else {
    WARN ("No extraction performed - must provide at least one node or edge of interest using command-line options");
  }

}
