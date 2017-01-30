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




#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/metric.h"
#include "dwi/tractography/connectome/tck2nodes.h"





namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {



using namespace App;



const char* modes[] = { "assignment_end_voxels", "assignment_radial_search", "assignment_reverse_search", "assignment_forward_search", "assignment_all_voxels", NULL };



const OptionGroup AssignmentOptions = OptionGroup ("Structural connectome streamline assignment option")

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

  Tck2nodes_base* tck2nodes = nullptr;
  for (size_t index = 0; modes[index]; ++index) {
    auto opt = get_options (modes[index]);
    if (opt.size()) {

      if (tck2nodes) {
        delete tck2nodes;
        tck2nodes = nullptr;
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




const OptionGroup MetricOptions = OptionGroup ("Structural connectome metric options")

  + Option ("scale_length", "scale each contribution to the connectome edge by the length of the streamline")
  + Option ("scale_invlength", "scale each contribution to the connectome edge by the inverse of the streamline length")
  + Option ("scale_invnodevol", "scale each contribution to the connectome edge by the inverse of the two node volumes")

  + Option ("scale_file", "scale each contribution to the connectome edge according to the values in a vector file")
    + Argument ("path").type_image_in();



void setup_metric (Metric& metric, Image<node_t>& nodes_data)
{
  if (get_options ("scale_length").size()) {
    if (get_options ("scale_invlength").size())
      throw Exception ("Options -scale_length and -scale_invlength are mutually exclusive");
    metric.set_scale_length();
  } else if (get_options ("scale_invlength").size()) {
    metric.set_scale_invlength();
  }
  if (get_options ("scale_invnnodevol").size())
    metric.set_scale_invnodevol (nodes_data);
  auto opt = get_options ("scale_file");
  if (opt.size())
    metric.set_scale_file (opt[0][0]);
}





}
}
}
}



