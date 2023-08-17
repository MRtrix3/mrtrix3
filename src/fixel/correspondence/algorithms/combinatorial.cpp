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

#include "fixel/correspondence/algorithms/combinatorial.h"

#include "fixel/correspondence/correspondence.h"


namespace MR {
  namespace Fixel {
    namespace Correspondence {
      namespace Algorithms {



        using namespace App;


        OptionGroup CombinatorialOptions = OptionGroup ("Options applicable to all combinatorial-based algorithms")

        + Option ("max_origins", "maximal number of origin source fixels for an individual target fixel "
                                 "(default: " + str(default_max_origins_per_target) + ")")
          + Argument ("value").type_integer (1)

        + Option ("max_objectives", "maximal number of objective target fixels for an individual source fixel "
                                    "(default: " + str(default_max_objectives_per_source) + ")")
          + Argument ("value").type_integer (1)

        + Option ("cost", "export a 3D image containing the optimal value of the relevant cost function in each voxel")
          + Argument ("path").type_image_out();



      }
    }
  }
}
