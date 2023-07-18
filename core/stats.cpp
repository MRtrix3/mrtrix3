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

#include "stats.h"

namespace MR
{

  namespace Stats
  {

    using namespace App;

    const char * field_choices[] = { "mean", "median", "std", "std_rv", "min", "max", "count", nullptr };

    const OptionGroup Options = OptionGroup ("Statistics options")
    + Option ("output",
        "output only the field specified. Multiple such options can be supplied if required. "
        "Choices are: " + join (field_choices, ", ") + ". Useful for use in scripts. "
        "Both std options refer to the unbiased (sample) standard deviation. "
        "For complex data, min, max and std are calculated separately for real and imaginary parts, "
        "std_rv is based on the real valued variance (equals sqrt of sum of variances of imaginary and real parts).").allow_multiple()
    + Argument ("field").type_choice (field_choices)

    + Option ("mask",
        "only perform computation within the specified binary mask image.")
    + Argument ("image").type_image_in ()

    + Option ("ignorezero",
        "ignore zero values during statistics calculation");

  }

}
