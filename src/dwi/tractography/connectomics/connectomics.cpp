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



#include "dwi/tractography/connectomics/connectomics.h"
#include "dwi/tractography/connectomics/edge_metrics.h"
#include "dwi/tractography/connectomics/tck2nodes.h"





namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectomics {



const char* metrics[] = { "count", "meanlength", "invlength", "invnodevolume", "invlength_invnodevolume", "mean_scalar", NULL };

const char* modes[] = { "voxel", "radial_search", "reverse_search", NULL };


using namespace App;



const OptionGroup AssignmentOption = OptionGroup ("Structural connectome streamline assignment option")

  + Option ("assignment_mode", "specify the mechanism by which streamlines are assigned to the relevant nodes. "
            "Options are: voxel, radial_search (default), reverse_search")
    + Argument ("choice").type_choice (modes)

  + Option ("assignment_distance", "set the distance threshold for streamline assignment (relevant for some modes, and behaviour depends on the particular assignment mode)")
    + Argument ("value").type_float (0.0, 0.0, 1e6);




Tck2nodes_base* load_assignment_mode (Image::Buffer<node_t>& nodes_data)
{
  int assignment_mode = 1; // default = radial search
  Options opt = get_options ("assignment_mode");
  if (opt.size())
    assignment_mode = opt[0][0];
  switch (assignment_mode) {
    case 0:
      return new Connectomics::Tck2nodes_voxel (nodes_data);
    case 1: case 2:
        opt = get_options ("assignment_distance");
        if (assignment_mode == 1)
          return new Connectomics::Tck2nodes_radial    (nodes_data, opt.size() ? to<float> (opt[0][0]) : TCK2NODES_RADIAL_DEFAULT_DIST);
        else
          return new Connectomics::Tck2nodes_revsearch (nodes_data, opt.size() ? to<float> (opt[0][0]) : TCK2NODES_REVSEARCH_DEFAULT_DIST);
    default:
      throw Exception ("Undefined streamline assigment mode");
  }
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

    case 0: return new Connectomics::Metric_count (); break;
    case 1: return new Connectomics::Metric_meanlength (); break;
    case 2: return new Connectomics::Metric_invlength (); break;
    case 3: return new Connectomics::Metric_invnodevolume (nodes_data); break;
    case 4: return new Connectomics::Metric_invlength_invnodevolume (nodes_data); break;

    case 5:
      opt = get_options ("image");
      if (!opt.size())
        throw Exception ("To use the \"mean_scalar\" metric, you must provide the associated scalar image using the -image option");
      return new Connectomics::Metric_meanscalar (opt[0][0]);
      break;

    default: throw Exception ("Undefined edge weight metric");

  }
}





}
}
}
}



