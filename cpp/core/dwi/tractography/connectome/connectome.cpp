/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/metric.h"
#include "dwi/tractography/connectome/tck2nodes.h"

namespace MR::DWI::Tractography::Connectome {

using namespace App;

const std::vector<std::string> assignment_options{"assignment_end_voxels",
                                                  "assignment_radial_search",
                                                  "assignment_reverse_search",
                                                  "assignment_forward_search",
                                                  "assignment_all_voxels"};

// clang-format off
const std::string tck2nodes_description
  = "The default mechanism by which streamlines are ascribed to connectome parcels"
    " is the \"radial search\" algorithm as described in reference (Smith et al., 2015),"
    " with a default maximal search distance of " + str(default_tck2nodes_radial_distance, 2) + "mm."
    " For each streamline endpoint,"
    " if there is no voxel with a non-zero parcel index"
    " whose centre is closer to the streamline endpoint than the maximal search distance,"
    " then that streamline endpoint will not be assigned to any parcel,"
    " and the streamline will be omitted from the connectome matrix"
    " (unless the -keep_unassigned option is specified)."
    " The maximal search distance can be modified using the -assignment_radial_search option,"
    " or an alternative algorithm can be activated using one of the other -assignment_* options.";

const OptionGroup AssignmentOptions =
    OptionGroup("Structural connectome streamline assignment options (can only specify one)")
    + Option("assignment_end_voxels",
             "use a simple voxel lookup value at each streamline endpoint")
    + Option("assignment_radial_search",
             "perform a radial search from each streamline endpoint to locate the nearest node."
             " Argument is the maximum radius in mm;"
             " if no node is found within this radius,"
             " the streamline endpoint is not assigned to any node."
             " Default search distance is " + str(default_tck2nodes_radial_distance, 2) + "mm.")
      + Argument("radius").type_float(0.0)
    + Option("assignment_reverse_search",
             "traverse from each streamline endpoint inwards along the streamline,"
             " in search of the last node traversed by the streamline."
             " Argument is the maximum traversal length in mm"
             " (set to 0 to allow search to continue to the streamline midpoint).")
      + Argument("max_dist").type_float(0.0)
    + Option("assignment_forward_search",
             "project the streamline forwards from the endpoint in search of a parcellation node voxel."
             " Argument is the maximum traversal length in mm.")
      + Argument("max_dist").type_float(0.0)
    + Option("assignment_all_voxels",
             "assign the streamline to all nodes it intersects along its length"
             " (note that this means a streamline may be assigned to more than two nodes,"
             " or indeed none at all)");
// clang-format on

std::unique_ptr<Tck2nodes_base> load_assignment_mode(Image<node_t> &nodes_data) {

  std::unique_ptr<Tck2nodes_base> tck2nodes;

  auto check_invalid = [](const std::unique_ptr<Tck2nodes_base> &ptr) {
    if (bool(ptr))
      throw Exception("Can only request one streamline assignment mechanism");
  };

  auto opt = get_options("assignment_end_voxels");
  if (!opt.empty()) {
    check_invalid(tck2nodes);
    tck2nodes.reset(new Tck2nodes_end_voxels(nodes_data));
  }
  opt = get_options("assignment_radial_search");
  if (!opt.empty()) {
    check_invalid(tck2nodes);
    tck2nodes.reset(new Tck2nodes_radial(nodes_data, static_cast<default_type>(opt[0][0])));
  }
  opt = get_options("assignment_reverse_search");
  if (!opt.empty()) {
    check_invalid(tck2nodes);
    tck2nodes.reset(new Tck2nodes_revsearch(nodes_data, static_cast<default_type>(opt[0][0])));
  }
  opt = get_options("assignment_forward_search");
  if (!opt.empty()) {
    check_invalid(tck2nodes);
    tck2nodes.reset(new Tck2nodes_forwardsearch(nodes_data, static_cast<default_type>(opt[0][0])));
  }
  opt = get_options("assignment_all_voxels");
  if (!opt.empty()) {
    check_invalid(tck2nodes);
    tck2nodes.reset(new Tck2nodes_all_voxels(nodes_data));
  }

  if (!tck2nodes)
    tck2nodes.reset(new Tck2nodes_radial(nodes_data, default_tck2nodes_radial_distance));

  return tck2nodes;
}

const OptionGroup MetricOptions =
    OptionGroup("Structural connectome metric options")

    + Option("scale_length", "scale each contribution to the connectome edge by the length of the streamline") +
    Option("scale_invlength",
           "scale each contribution to the connectome edge by the inverse of the streamline length") +
    Option("scale_invnodevol", "scale each contribution to the connectome edge by the inverse of the two node volumes")

    + Option("scale_file", "scale each contribution to the connectome edge according to the values in a vector file") +
    Argument("path").type_image_in();

void setup_metric(Metric &metric, Image<node_t> &nodes_data) {
  if (!get_options("scale_length").empty()) {
    if (!get_options("scale_invlength").empty())
      throw Exception("Options -scale_length and -scale_invlength are mutually exclusive");
    metric.set_scale_length();
  } else if (!get_options("scale_invlength").empty()) {
    metric.set_scale_invlength();
  }
  if (!get_options("scale_invnodevol").empty())
    metric.set_scale_invnodevol(nodes_data);
  auto opt = get_options("scale_file");
  if (!opt.empty()) {
    try {
      metric.set_scale_file(opt[0][0]);
    } catch (Exception &e) {
      throw Exception(e,
                      "-scale_file option expects a file containing a list of numbers (one for each streamline); "
                      "file \"" +
                          std::string(opt[0][0]) + "\" does not appear to contain this");
    }
  }
}

} // namespace MR::DWI::Tractography::Connectome
