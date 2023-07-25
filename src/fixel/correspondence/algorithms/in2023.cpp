/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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

#include "fixel/correspondence/algorithms/in2023.h"

#include "app.h"

namespace MR {
  namespace Fixel {
    namespace Correspondence {
      namespace Algorithms {



        using namespace App;

        OptionGroup IN2023Options = OptionGroup ("Options specific to algorithm \"in2023\"")
        + Option ("constants", "set values for the two constants that modulate the influence of different cost function terms")
          + Argument ("alpha").type_float (0.0)
          + Argument ("beta").type_float (0.0);



        float IN2023::a = default_in2023_alpha;
        float IN2023::b = default_in2023_beta;


      }
    }
  }
}
