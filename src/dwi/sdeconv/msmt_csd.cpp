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

#include "dwi/sdeconv/msmt_csd.h"

namespace MR
{
  namespace DWI
  {
    namespace SDeconv
    {

    using namespace App;

    const OptionGroup MSMT_CSD_options =
      OptionGroup ("Options for the Multi-Shell, Multi-Tissue Constrained Spherical Deconvolution algorithm")
      + Option ("norm_lambda",
                "the regularisation parameter lambda that controls the strength of the "
                "constraint on the norm of the solution (default = " + str(DEFAULT_MSMTCSD_NORM_LAMBDA, 2) + ").")
      + Argument ("value").type_float (0.0)

      + Option ("neg_lambda",
                "the regularisation parameter lambda that controls the strength of the "
                "non-negativity constraint (default = " + str(DEFAULT_MSMTCSD_NEG_LAMBDA, 2) + ").")
      + Argument ("value").type_float (0.0)

      + Option ("predicted_signal",
                "output the predicted dwi image.")
      + Argument ("image").type_image_out();


    }
  }
}

