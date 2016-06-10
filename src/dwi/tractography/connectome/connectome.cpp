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



#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/edge_metrics.h"
#include "dwi/tractography/connectome/tck2nodes.h"





namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {



const char* modes[] = { "assignment_end_voxels", "assignment_radial_search", "assignment_reverse_search", "assignment_forward_search", "assignment_all_voxels", NULL };

const char* metrics[] = { "count", "meanlength", "invlength", "invnodevolume", "invlength_invnodevolume", "mean_scalar", NULL };


using namespace App;



const OptionGroup AssignmentOption = OptionGroup ("Structural connectome streamline assignment option")

  + Option ("assignment_end_voxels", "use a simple voxel lookup value at each streamline endpoint")

  + Option ("assignment_radial_search", "perform a radial search from each streamline endpoint to locate the nearest node.\n"
                                        "Argument is the maximum radius in mm; if no node is found within this radius, the streamline endpoint is not assigned to any node. "
                                        "Default search distance is " + str(TCK2NODES_RADIAL_DEFAULT_DIST, 2) + "mm.")
    + Argument ("radius").type_float (0.0)

  + Option ("assignment_reverse_search", "traverse from each streamline endpoint inwards along the streamline, in search of the last node traversed by the streamline. "
                                         "Argument is the maximum traversal length in mm (set to 0 to allow search to continue to the streamline midpoint).")
    + Argument ("max_dist").type_float (0.0)

  + Option ("assignment_forward_search", "project the streamline forwards from the endpoint in search of a parcellation node voxel. "
                                         "Argument is the maximum traversal length in mm.")
    + Argument ("max_dist").type_float (0.0)

  + Option ("assignment_all_voxels", "assign the streamline to all nodes it intersects along its length "
                                     "(note that this means a streamline may be assigned to more than two nodes, or indeed none at all)");




Tck2nodes_base* load_assignment_mode (Image<node_t>& nodes_data)
{

  Tck2nodes_base* tck2nodes = NULL;
  for (size_t index = 0; modes[index]; ++index) {
    auto opt = get_options (modes[index]);
    if (opt.size()) {

      if (tck2nodes) {
        delete tck2nodes;
        tck2nodes = NULL;
        throw Exception ("Please only request one streamline assignment mechanism");
      }

      switch (index) {
        case 0: tck2nodes = new Tck2nodes_end_voxels (nodes_data); break;
        case 1: tck2nodes = new Tck2nodes_radial (nodes_data, float(opt[0][0])); break;
        case 2: tck2nodes = new Tck2nodes_revsearch (nodes_data, float(opt[0][0])); break;
        case 3: tck2nodes = new Tck2nodes_forwardsearch (nodes_data, float(opt[0][0])); break;
        case 4: tck2nodes = new Tck2nodes_all_voxels (nodes_data); break;
      }

    }
  }

  // default
  if (!tck2nodes)
    tck2nodes = new Tck2nodes_radial (nodes_data, TCK2NODES_RADIAL_DEFAULT_DIST);

  return tck2nodes;

}




const OptionGroup MetricOption = OptionGroup ("Structural connectome metric option")

  + Option ("metric", "specify the edge weight metric. "
                      "Options are: count (default), meanlength, invlength, invnodevolume, invlength_invnodevolume, mean_scalar")
    + Argument ("choice").type_choice (metrics)

  + Option ("image", "provide the associated image for the mean_scalar metric")
    + Argument ("path").type_image_in();



Metric_base* load_metric (Image<node_t>& nodes_data)
{
  int edge_metric = 0; // default = count
  auto opt = get_options ("metric");
  if (opt.size())
    edge_metric = opt[0][0];
  switch (edge_metric) {

    case 0: return new Metric_count (); break;
    case 1: return new Metric_meanlength (); break;
    case 2: return new Metric_invlength (); break;
    case 3: return new Metric_invnodevolume (nodes_data); break;
    case 4: return new Metric_invlength_invnodevolume (nodes_data); break;

    case 5:
      opt = get_options ("image");
      if (!opt.size())
        throw Exception ("To use the \"mean_scalar\" metric, you must provide the associated scalar image using the -image option");
      return new Metric_meanscalar (opt[0][0]);
      break;

    default: throw Exception ("Undefined edge weight metric");

  }
  return NULL;
}





}
}
}
}



