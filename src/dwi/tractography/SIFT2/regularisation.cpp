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
      const char* reg_fn_choices[] = { "coefficient", "factor", "gamma", nullptr };


      OptionGroup RegularisationOptions = OptionGroup ("Options relating to SIFT2 algorithm regularisation")

      // TODO Transfer a primary explanation of the different regularisation bases to a Description paragraph
      + Option ("reg_basis_abs", "The basis upon which regularisation is applied when optimising absolute weights; "
                                 "options are: "
                                 "streamline (regularisation is applied independently to each streamline), "
                                 "fixel (regularisation drives those streamlines traversing each fixel toward a common value) (default)")
        + Argument ("choice").type_choice(reg_basis_choices)

      + Option ("reg_fn_abs", "The form of the regularisation function when optimising absolute weights; "
                              "options are: "
                              "coefficient (quadratically penalise difference of exponential coefficients to zero / fixel mean); "
                              "factor (quadratically penalise difference of streamline weights to one / fixel mean); "
                              "gamma (penalises coefficient if down-reulating, factor if up-regulating) (default)")
        + Argument ("choice").type_choice(reg_fn_choices)

      + Option ("reg_strength_abs", "modulate strength of applied regularisation when optimising absolute weights "
                                    "(default: " + str(regularisation_strength_abs_default, 2) + ")")
        + Argument ("value").type_float (0.0)

      + Option ("reg_basis_diff", "The basis upon which regularisation is applied when optimising differential weights; "
                                  "options are: "
                                  "streamline (regularisation is applied independently to each streamline) (default), "
                                  "fixel (regularisation drives those streamlines traversing each fixel toward a common value)")
        + Argument ("choice").type_choice(reg_basis_choices)

      + Option ("reg_strength_diff", "modulate strength of applied regularisation when optimising differential weights "
                                     "(default: " + str(regularisation_strength_diff_default, 2) + ")")
        + Argument ("value").type_float (0.0);



      }
    }
  }
}
