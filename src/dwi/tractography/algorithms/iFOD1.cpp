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

#include "dwi/tractography/algorithms/iFOD1.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Algorithms
      {

        using namespace App;

        const OptionGroup iFODOptions = OptionGroup ("Options specific to the iFOD tracking algorithms")

        + Option ("power", "raise the FOD to the power specified (defaults are: 1.0 for iFOD1; 1.0/nsamples for iFOD2).")
          + Argument ("value").type_float (0.0);


        void load_iFOD_options (Tractography::Properties& properties)
        {
          auto opt = get_options ("power");
          if (opt.size()) properties["fod_power"] = str<float> (opt[0][0]);
        }

      }
    }
  }
}


