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

#include "dwi/tractography/tracking/tractography.h"
#include <fmt/format.h>

namespace MR::DWI::Tractography::Tracking {

using namespace App;
// clang-format off
const OptionGroup TrackOption =
    OptionGroup("Streamlines tractography options")

    + Option("select",
             fmt::format("set the desired number of streamlines to be selected by tckgen,"
                         " after all selection criteria have been applied"
                         " (i.e. inclusion/exclusion ROIs, min/max length, etc)."
                         " tckgen will keep seeding streamlines until this number of streamlines have been selected,"
                         " or the maximum allowed number of seeds has been exceeded"
                         " (see -seeds option)."
                         " By default, {} streamlines are to be selected."
                         " Set to zero to disable,"
                         " which will result in streamlines being seeded"
                         " until the number specified by -seeds has been reached.",
                         Defaults::num_selected_tracks))
      + Argument("number").type_integer(0)

    + Option("step",
             fmt::format("set the step size of the algorithm in mm"
                         " (defaults:"
                         " for first-order algorithms, {:.2g} x voxelsize;"
                         " if using RK4, {:.2g} x voxelsize;"
                         " for iFOD2: {:.2g} x voxelsize).",
                         Defaults::stepsize_voxels_firstorder,
                         Defaults::stepsize_voxels_rk4,
                         Defaults::stepsize_voxels_ifod2))
      + Argument("size").type_float(0.0)

    + Option("angle",
             fmt::format("set the maximum angle in degrees between successive steps"
                         " (defaults:"
                         " {} for deterministic algorithms;"
                         " {} for iFOD1 / nulldist1;"
                         " {} for iFOD2 / nulldist2)",
                         Defaults::angle_deterministic,
                         Defaults::angle_ifod1,
                         Defaults::angle_ifod2))
      + Argument("theta").type_float(0.0)

    + Option("minlength",
             fmt::format("set the minimum length of any track in mm"
                         " (defaults:"
                         " without ACT, {:.2g} x voxelsize;"
                         " with ACT, {:.2g} x voxelsize).",
                         Defaults::minlength_voxels_noact,
                         Defaults::minlength_voxels_withact))
      + Argument("value").type_float(0.0)

    + Option("maxlength",
             fmt::format("set the maximum length of any track in mm"
                         " (default: {} x voxelsize).",
                         Defaults::maxlength_voxels))
      + Argument("value").type_float(0.0)

    + Option("cutoff",
             fmt::format("set the FOD amplitude / fixel size / tensor FA cutoff for terminating tracks"
                         " (defaults:"
                         " {:.2g} for FOD-based algorithms;"
                         " {:.2g} for fixel-based algorithms;"
                         " {:.2g} for tensor-based algorithms;"
                         " threshold multiplied by {} when using ACT).",
                         Defaults::cutoff_fod,
                         Defaults::cutoff_fixel,
                         Defaults::cutoff_fa,
                         Defaults::cutoff_act_multiplier))
      + Argument("value").type_float(0.0)

    + Option("trials",
             fmt::format("set the maximum number of sampling trials at each point"
                         " (only used for iFOD1 / iFOD2)"
                         " (default: {}).",
                         Defaults::max_trials_per_step))
      + Argument("number").type_integer(1)

    + Option("noprecomputed",
             "do NOT pre-compute legendre polynomial values."
             " Warning: this will slow down the algorithm by a factor of approximately 4.")

    + Option("rk4",
             "use 4th-order Runge-Kutta integration"
             " (slower, but eliminates curvature overshoot in 1st-order deterministic methods)")

    + Option("stop",
             "stop propagating a streamline once it has traversed all include regions")

    + Option("downsample",
             "downsample the generated streamlines to reduce output file size"
             " (default is (samples-1) for iFOD2,"
             " no downsampling for all other algorithms)")
      + Argument("factor").type_integer(1);
// clang-format on

/**
Loads properties related to streamlines AND loads include etc ROIs.
*/
void load_streamline_properties_and_rois(Properties &properties) {

  // Validity check
  if (!get_options("include_ordered").empty() && get_options("seed_unidirectional").empty())
    throw Exception("-include_ordered requires that -seed_unidirectional is set, but this is not so");

  using namespace MR::App;

  auto opt = get_options("select");
  if (!opt.empty())
    properties["max_num_tracks"] = fmt::format("{}", static_cast<unsigned int>(opt[0][0]));

  opt = get_options("step");
  if (!opt.empty())
    properties["step_size"] = std::string(opt[0][0]);

  opt = get_options("angle");
  if (!opt.empty())
    properties["max_angle"] = std::string(opt[0][0]);

  opt = get_options("minlength");
  if (!opt.empty())
    properties["min_dist"] = std::string(opt[0][0]);

  opt = get_options("maxlength");
  if (!opt.empty())
    properties["max_dist"] = std::string(opt[0][0]);

  opt = get_options("cutoff");
  if (!opt.empty())
    properties["threshold"] = std::string(opt[0][0]);

  opt = get_options("trials");
  if (!opt.empty())
    properties["max_trials"] = fmt::format("{}", static_cast<unsigned int>(opt[0][0]));

  opt = get_options("noprecomputed");
  if (!opt.empty())
    properties["sh_precomputed"] = "0";

  opt = get_options("rk4");
  if (!opt.empty())
    properties["rk4"] = "1";

  load_rois(properties); // rois must be loaded before stop parameter in order to check its validity

  opt = get_options("stop");
  if (!opt.empty()) {
    if (properties.include.size() || properties.ordered_include.size())
      properties["stop_on_all_include"] = "1";
    else
      WARN("-stop option ignored - no inclusion regions specified");
  }

  opt = get_options("downsample");
  if (!opt.empty())
    properties["downsample_factor"] = fmt::format("{}", static_cast<unsigned int>(opt[0][0]));

  opt = get_options("grad");
  if (!opt.empty())
    properties["DW_scheme"] = std::string(opt[0][0]);
}

} // namespace MR::DWI::Tractography::Tracking
