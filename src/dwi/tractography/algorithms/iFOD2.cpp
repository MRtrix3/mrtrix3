/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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
 * See the Mozila Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "dwi/tractography/algorithms/iFOD2.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Algorithms
      {

        using namespace App;

        const OptionGroup iFOD2Option = OptionGroup ("Options specific to the iFOD2 tracking algorithm")

        + Option ("samples",
                  "set the number of FOD samples to take per step (Default: " + str(TCKGEN_DEFAULT_IFOD2_NSAMPLES) + ").")
          + Argument ("number").type_integer (2, 100)

        + Option ("power", "raise the FOD to the power specified (default is 1/nsamples).")
          + Argument ("value").type_float (0.0);


        void load_iFOD2_options (Tractography::Properties& properties)
        {
          auto opt = get_options ("samples");
          if (opt.size()) properties["samples_per_step"] = str<unsigned int> (opt[0][0]);

          opt = get_options ("power");
          if (opt.size()) properties["fod_power"] = str<float> (opt[0][0]);
        }

      }
    }
  }
}


