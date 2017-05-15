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


#include "stats.h"

namespace MR
{

  namespace Stats
  {

    using namespace App;

    const char * field_choices[] = { "mean", "median", "std", "min", "max", "count", NULL };

    const OptionGroup Options = OptionGroup ("Statistics options")
    + Option ("output",
        "output only the field specified. Multiple such options can be supplied if required. "
        "Choices are: " + join (field_choices, ", ") + ". Useful for use in scripts.").allow_multiple()
    + Argument ("field").type_choice (field_choices)

    + Option ("mask",
        "only perform computation within the specified binary mask image.")
    + Argument ("image").type_image_in ()

    + Option ("ignorezero",
        "ignore zero values during statistics calculation");

  }

}
