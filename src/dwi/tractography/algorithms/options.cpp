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

#include "dwi/tractography/algorithms/options.h"

#include "dwi/tractography/tracking/tractography.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Algorithms
      {



        using namespace App;



        const OptionGroup FODOptions = OptionGroup ("Options specific to tracking algorithms that sample from FODs (iFOD1, iFOD2, PTT)")
        // TODO Refine PTT default following implementation changes
        + Option ("power", "raise the FOD to the power specified (defaults are: 1.0 for iFOD1, PTT; 1.0/nsamples for iFOD2).")
          + Argument ("value").type_float (0.0)

        + Option ("trials", "set the maximum number of sampling trials at each point (default: " + str(Tracking::Defaults::max_trials_per_step) + ").")
          + Argument ("number").type_integer (1)

        + Option ("noprecomputed", "do NOT pre-compute legendre polynomial values. "
                                   "Warning: this will slow down the algorithm by a factor of approximately 4.");


        const OptionGroup SecondOrderOptions = OptionGroup ("Options specific to 2nd-order integration methods (iFOD2, PTT)")
        + Option ("samples", "set the number of FOD samples to take per step (Default: " + str(Tracking::Defaults::secondorder_nsamples) + ").")
          + Argument ("number").type_integer (2, 100);



        void load_FOD_options (Tractography::Properties& properties)
        {
          auto opt = get_options ("power");
          if (opt.size())
            properties["fod_power"] = str<float> (opt[0][0]);

          opt = get_options ("trials");
          if (opt.size())
            properties["max_trials"] = str<unsigned int> (opt[0][0]);

          opt = get_options ("noprecomputed");
            if (opt.size()) properties["sh_precomputed"] = "0";
        }

        void load_2ndorder_options (Tractography::Properties& properties)
        {
          auto opt = get_options ("samples");
          if (opt.size())
            properties["samples_per_step"] = str<unsigned int> (opt[0][0]);
        }



      }
    }
  }
}


