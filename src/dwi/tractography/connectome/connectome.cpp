/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2012.

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



#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/edge_metrics.h"
#include "dwi/tractography/connectome/tck2nodes.h"





namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {



const char* modes[] = { "assignment_voxel_lookup", "assignment_radial_search", "assignment_reverse_search", "assignment_forward_search", NULL };

const char* metrics[] = { "count", "meanlength", "invlength", "invnodevolume", "invlength_invnodevolume", "mean_scalar", NULL };


using namespace App;



const OptionGroup AssignmentOption = OptionGroup ("Structural connectome streamline assignment option")

  + Option ("assignment_voxel_lookup", "use a simple voxel lookup value at each streamline endpoint")

  + Option ("assignment_radial_search", "perform a radial search from each streamline endpoint to locate the nearest node.\n"
                                        "Argument is the maximum radius in mm; if no node is found within this radius, the streamline endpoint is not assigned to any node. ")
    + Argument ("radius").type_float (0.0, TCK2NODES_RADIAL_DEFAULT_DIST, 1e6)

  + Option ("assignment_reverse_search", "traverse from each streamline endpoint inwards along the streamline, in search of the last node traversed by the streamline. "
                                         "Argument is the maximum traversal length in mm (set to 0 to allow search to continue to the streamline midpoint).")
    + Argument ("max_dist").type_float (0.0, TCK2NODES_REVSEARCH_DEFAULT_DIST, 1e6)

  + Option ("assignment_forward_search", "project the streamline forwards from the endpoint in search of a parcellation node voxel. "
                                         "Argument is the maximum traversal length in mm.")
    + Argument ("max_dist").type_float (0.0, TCK2NODES_FORWARDSEARCH_DEFAULT_DIST, 1e6);




Tck2nodes_base* load_assignment_mode (Image::Buffer<node_t>& nodes_data)
{

  Tck2nodes_base* tck2nodes = NULL;
  for (size_t index = 0; modes[index]; ++index) {
    Options opt = get_options (modes[index]);
    if (opt.size()) {

      if (tck2nodes) {
        delete tck2nodes;
        tck2nodes = NULL;
        throw Exception ("Please only request one streamline assignment mechanism");
      }

      switch (index) {
        case 0: tck2nodes = new Tck2nodes_voxel         (nodes_data); break;
        case 1: tck2nodes = new Tck2nodes_radial        (nodes_data, float(opt[0][0])); break;
        case 2: tck2nodes = new Tck2nodes_revsearch     (nodes_data, float(opt[0][0])); break;
        case 3: tck2nodes = new Tck2nodes_forwardsearch (nodes_data, float(opt[0][0])); break;
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



Metric_base* load_metric (Image::Buffer<node_t>& nodes_data)
{
  int edge_metric = 0; // default = count
  Options opt = get_options ("metric");
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



