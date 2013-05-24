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
#include "ptr.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/connectomics/connectomics.h"
#include "dwi/tractography/connectomics/edge_metrics.h"
#include "dwi/tractography/connectomics/multithread.h"
#include "dwi/tractography/connectomics/tck2nodes.h"


#include "image/buffer_preload.h"
#include "image/loop.h"
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
  + "generate a connectome matrix from a streamlines file and a segmented anatomical image";

  ARGUMENTS
  + Argument ("tracks_in",      "the input track file").type_file()
  + Argument ("nodes_in",       "the input segmented anatomical image").type_image_in()
  + Argument ("connectome_out", "the output .csv file containing edge weights").type_file();


  OPTIONS
  + Connectomics::AssignmentOption
  + Connectomics::MetricOption;

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
  Image::Loop loop;
  for (loop.start (nodes); loop.ok(); loop.next (nodes)) {
    if (nodes.value() > max_node_index)
      max_node_index = nodes.value();
  }

  // Multi-threaded connectome construction
  Mapping::TrackLoader loader (reader, properties["count"].empty() ? 0 : to<size_t>(properties["count"]), "Constructing connectome... ");
  Mapper mapper (*tck2nodes, *metric);
  Connectome connectome (max_node_index);
  Thread::run_batched_queue_threaded_pipe (loader, Mapping::TrackAndIndex(), 100, mapper, Mapped_track(), 100, connectome);

  if (metric->scale_edges_by_streamline_count())
    connectome.scale_by_streamline_count();

  connectome.write (argument[2]);

}
