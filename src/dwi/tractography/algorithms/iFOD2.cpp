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

        const OptionGroup iFOD2Options = OptionGroup ("Options specific to the iFOD2 tracking algorithm")

        + Option ("samples",
                  "set the number of FOD samples to take per step (Default: " + str(Tracking::Defaults::ifod2_nsamples) + ").")
          + Argument ("number").type_integer (2, 100);


        void load_iFOD2_options (Tractography::Properties& properties)
        {
          auto opt = get_options ("samples");
          if (opt.size()) properties["samples_per_step"] = str<unsigned int> (opt[0][0]);
        }

      }
    }
  }
}


