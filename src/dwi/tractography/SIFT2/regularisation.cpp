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



#include "dwi/tractography/SIFT2/regularisation.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT2
      {



      using namespace App;



      const char* reg_basis_choices[] = { "streamline", "fixel", nullptr };
      const char* reg_fn_choices[] = { "coefficient", "weight", "gamma", nullptr };


      OptionGroup RegularisationOptions = OptionGroup ("Options relating to regularisation")

      + Option ("reg_basis", "The basis upon which regularisation is applied; "
                             "options are: "
                             "streamline (regularisation is applied independently to each streamline), "
                             "fixel (regularisation drives those streamlines traversing each fixel toward a common value) (default)")
        + Argument ("choice").type_choice(reg_basis_choices)

      + Option ("reg_fn", "The form of the regularisation function; "
                          "options are: "
                          "coefficient (quadratically penalise difference of exponential coefficients to zero / fixel mean); "
                          "weight (quadratically penalise difference of streamline weights to one / fixel mean); "
                          "gamma (penalises coefficient if down-reulating, weight if up-regulating) (default)")
        + Argument ("choice").type_choice(reg_fn_choices)

      + Option ("reg_strength", "modulate strength of applied regularisation (default: " + str(regularisation_strength_default, 2) + ")")
        + Argument ("value").type_float (0.0);




      }
    }
  }
}
