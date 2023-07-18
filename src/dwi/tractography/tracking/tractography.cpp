/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {

      using namespace App;

      const OptionGroup TrackOption = OptionGroup ("Streamlines tractography options")

      + Option ("select",
            "set the desired number of streamlines to be selected by "
            "tckgen, after all selection criteria have been applied "
            "(i.e. inclusion/exclusion ROIs, min/max length, etc). "
            "tckgen will keep seeding streamlines until this number of "
            "streamlines have been selected, or the maximum allowed "
            "number of seeds has been exceeded (see -seeds option). "
            "By default, " + str(Defaults::num_selected_tracks) + " "
            "streamlines are to be selected. "
            "Set to zero to disable, which will result in streamlines "
            "being seeded until the number specified by -seeds has been "
            "reached.")
          + Argument ("number").type_integer (0)

      + Option ("step",
            "set the step size of the algorithm in mm (defaults: "
            "for first-order algorithms, " + str(Defaults::stepsize_voxels_firstorder, 2) + " x voxelsize; " +
            "if using RK4, " + str(Defaults::stepsize_voxels_rk4, 2) + " x voxelsize; "
            "for iFOD2: " + str(Defaults::stepsize_voxels_ifod2, 2) + " x voxelsize).")
          + Argument ("size").type_float (0.0)

      + Option ("angle",
            "set the maximum angle in degrees between successive steps (defaults: "
            + str(Defaults::angle_deterministic) + " for deterministic algorithms; "
            + str(Defaults::angle_ifod1) + " for iFOD1 / nulldist1; "
            + str(Defaults::angle_ifod2) + " for iFOD2 / nulldist2)")
          + Argument ("theta").type_float (0.0)

      + Option ("minlength",
            "set the minimum length of any track in mm (defaults: "
            "without ACT, " + str(Defaults::minlength_voxels_noact) + " x voxelsize; " +
            "with ACT, " + str(Defaults::minlength_voxels_withact) + " x voxelsize).")
          + Argument ("value").type_float (0.0)

      + Option ("maxlength",
            "set the maximum length of any track in mm "
            "(default: " + str(Defaults::maxlength_voxels) + " x voxelsize).")
          + Argument ("value").type_float (0.0)

      + Option ("cutoff",
            "set the FOD amplitude / fixel size / tensor FA cutoff for terminating tracks "
            "(defaults: " +
            str(Defaults::cutoff_fod, 2) + " for FOD-based algorithms; " +
            str(Defaults::cutoff_fixel, 2) + " for fixel-based algorithms; " +
            str(Defaults::cutoff_fa, 2) + " for tensor-based algorithms; " +
            "threshold multiplied by " + str(Defaults::cutoff_act_multiplier) + " when using ACT).")
          + Argument ("value").type_float (0.0)

      + Option ("trials",
            "set the maximum number of sampling trials at each point "
            "(only used for iFOD1 / iFOD2) "
            "(default: " + str(Defaults::max_trials_per_step) + ").")
          + Argument ("number").type_integer (1)

      + Option ("noprecomputed",
            "do NOT pre-compute legendre polynomial values. Warning: "
            "this will slow down the algorithm by a factor of approximately 4.")

      + Option ("rk4", "use 4th-order Runge-Kutta integration "
                       "(slower, but eliminates curvature overshoot in 1st-order deterministic methods)")

      + Option ("stop", "stop propagating a streamline once it has traversed all include regions")

      + Option ("downsample", "downsample the generated streamlines to reduce output file size "
                              "(default is (samples-1) for iFOD2, no downsampling for all other algorithms)")
          + Argument ("factor").type_integer (1);


      /**
      Loads properties related to streamlines AND loads include etc ROIs.
      */
      void load_streamline_properties_and_rois (Properties& properties)
      {

         //Validity check
         if (get_options("include_ordered").size() && !get_options("seed_unidirectional").size())
            throw Exception("-include_ordered requires that -seed_unidirectional is set, but this is not so");


        using namespace MR::App;

        auto opt = get_options ("select");
        if (opt.size()) properties["max_num_tracks"] = str<unsigned int> (opt[0][0]);

        opt = get_options ("step");
        if (opt.size()) properties["step_size"] = std::string (opt[0][0]);

        opt = get_options ("angle");
        if (opt.size()) properties["max_angle"] = std::string (opt[0][0]);

        opt = get_options ("minlength");
        if (opt.size()) properties["min_dist"] = std::string (opt[0][0]);

        opt = get_options ("maxlength");
        if (opt.size()) properties["max_dist"] = std::string (opt[0][0]);

        opt = get_options ("cutoff");
        if (opt.size()) properties["threshold"] = std::string (opt[0][0]);

        opt = get_options ("trials");
        if (opt.size()) properties["max_trials"] = str<unsigned int> (opt[0][0]);

        opt = get_options ("noprecomputed");
        if (opt.size()) properties["sh_precomputed"] = "0";

        opt = get_options ("rk4");
        if (opt.size()) properties["rk4"] = "1";

        load_rois(properties);//rois must be loaded before stop parameter in order to check its validity

        opt = get_options ("stop");
        if (opt.size()) {
          if (properties.include.size() || properties.ordered_include.size())
            properties["stop_on_all_include"] = "1";
          else
            WARN ("-stop option ignored - no inclusion regions specified");
        }

        opt = get_options ("downsample");
        if (opt.size()) properties["downsample_factor"] = str<unsigned int> (opt[0][0]);

        opt = get_options ("grad");
        if (opt.size()) properties["DW_scheme"] = std::string (opt[0][0]);

      }



      }
    }
  }
}


