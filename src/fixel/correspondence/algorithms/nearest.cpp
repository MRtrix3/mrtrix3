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

#include "fixel/correspondence/algorithms/nearest.h"

#include "app.h"
#include "fixel/correspondence/correspondence.h"

namespace MR {
  namespace Fixel {
    namespace Correspondence {
      namespace Algorithms {



        using namespace App;

        OptionGroup NearestOptions = OptionGroup ("Options specific to algorithm \"nearest\"")
        + Option ("angle", "maximum angle within which a corresponding fixel may be selected, in degrees "
                           "(default: " + str(default_nearest_maxangle) + ")")
          + Argument ("value").type_float (0.0f, 90.0f);



      }
    }
  }
}
