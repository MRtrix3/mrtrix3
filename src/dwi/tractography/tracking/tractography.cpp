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
            "By default, 1000 streamlines are to be selected. "
            "Set to zero to disable, which will result in streamlines "
            "being seeded until the number specified by -seeds has been "
            "reached.")
          + Argument ("number").type_integer (0)

      + Option ("step",
            "set the step size of the algorithm in mm (default is 0.1 x voxelsize; for iFOD2: 0.5 x voxelsize).")
          + Argument ("size").type_float (0.0)

      + Option ("angle",
            "set the maximum angle between successive steps (default is 90deg x stepsize / voxelsize).")
          + Argument ("theta").type_float (0.0)

      + Option ("minlength",
            "set the minimum length of any track in mm "
            "(default is 5 x voxelsize without ACT, 2 x voxelsize with ACT).")
          + Argument ("value").type_float (0.0)

      + Option ("maxlength",
            "set the maximum length of any track in mm (default is 100 x voxelsize).")
          + Argument ("value").type_float (0.0)

      + Option ("cutoff",
            "set the FA or FOD amplitude cutoff for terminating tracks "
            "(default is " + str(TCKGEN_DEFAULT_CUTOFF, 2) + ").")
          + Argument ("value").type_float (0.0)

      + Option ("trials",
            "set the maximum number of sampling trials at each point (only "
            "used for probabilistic tracking).")
          + Argument ("number").type_integer (1)

      + Option ("noprecomputed",
            "do NOT pre-compute legendre polynomial values. Warning: "
            "this will slow down the algorithm by a factor of approximately 4.")

      + Option ("power",
            "raise the FOD to the power specified (default is 1/nsamples).")
          + Argument ("value").type_float (0.0)

      + Option ("samples",
            "set the number of FOD samples to take per step for the 2nd order "
            "(iFOD2) method (Default: " + str(TCKGEN_DEFAULT_IFOD2_NSAMPLES) + ").")
          + Argument ("number").type_integer (2, 100)

      + Option ("rk4", "use 4th-order Runge-Kutta integration "
                       "(slower, but eliminates curvature overshoot in 1st-order deterministic methods)")

      + Option ("stop", "stop propagating a streamline once it has traversed all include regions")

      + Option ("downsample", "downsample the generated streamlines to reduce output file size "
                              "(default is (samples-1) for iFOD2, no downsampling for all other algorithms)")
          + Argument ("factor").type_integer (2);



      void load_streamline_properties (Properties& properties)
      {

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

        opt = get_options ("power");
        if (opt.size()) properties["fod_power"] = std::string (opt[0][0]);

        opt = get_options ("samples");
        if (opt.size()) properties["samples_per_step"] = str<unsigned int> (opt[0][0]);

        opt = get_options ("rk4");
        if (opt.size()) properties["rk4"] = "1";

        opt = get_options ("include");
        for (size_t i = 0; i < opt.size(); ++i)
          properties.include.add (ROI (opt[i][0]));

        opt = get_options ("exclude");
        for (size_t i = 0; i < opt.size(); ++i)
          properties.exclude.add (ROI (opt[i][0]));

        opt = get_options ("mask");
        for (size_t i = 0; i < opt.size(); ++i)
          properties.mask.add (ROI (opt[i][0]));

        opt = get_options ("stop");
        if (opt.size()) {
          if (properties.include.size())
            properties["stop_on_all_include"] = "1";
          else
            WARN ("-stop option ignored - no -include regions specified");
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


