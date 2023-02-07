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

#include "dwi/tractography/algorithms/newAlgorithm.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Algorithms
      {

        using namespace App;

        const OptionGroup newAlgorithmOptions = OptionGroup ("Options specific to the new tracking algorithm")

        + Option ("response_function",
                  "Insert the location of the response function: ")
          + Argument ("file_path").type_file_in ();


        void load_new_algorithm_options (Tractography::Properties& properties)
        {
          auto opt = get_options ("response_function");
          if (opt.size()) properties["response_function_path"] = str(opt[0][0]);
        }

      }
    }
  }
}


