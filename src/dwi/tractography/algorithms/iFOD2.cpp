/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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


