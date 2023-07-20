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

#include "dwi/tractography/algorithms/ptt.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Algorithms
      {



        using namespace App;



        const OptionGroup PTTOptions = OptionGroup ("Options specific to the Parallel Transport Tractography (PTT) algorithm")
        + Option ("probe_length", "length of probe used to sample FOD amplitudes; not necessarily equal to step size (default: " + str(Defaults::probelength_voxels_ptt) + " x voxel size)")
          + Argument ("value").type_float (0.0);



        void load_PTT_options (Tractography::Properties& properties)
        {
          auto opt = get_options ("probe_length");
          if (opt.size())
            properties["probe_length"] = str<float> (opt[0][0]);
        }



      }
    }
  }
}


