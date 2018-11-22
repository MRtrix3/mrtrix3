/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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
 * See the Mozila Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "stats/permtest.h"

namespace MR
{
  namespace Stats
  {
    namespace PermTest
    {



      const App::OptionGroup Options (const bool include_nonstationarity)
      {
        using namespace App;

        OptionGroup result = OptionGroup ("Options for permutation testing")
          + Option ("notest", "don't perform permutation testing and only output population statistics (effect size, stdev etc)")
          + Option ("nperms", "the number of permutations (Default: " + str(DEFAULT_NUMBER_PERMUTATIONS) + ")")
            + Argument ("num").type_integer (1)
          + Option ("permutations", "manually define the permutations (relabelling). The input should be a text file defining a m x n matrix, "
                                    "where each relabelling is defined as a column vector of size    m, and the number of columns, n, defines "
                                    "the number of permutations. Can be generated with the palm_quickperms function in PALM (http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM). "
                                    "Overrides the nperms option.")
            + Argument ("file").type_file_in();

        if (include_nonstationarity) {
          result
          + Option ("nonstationary", "perform non-stationarity correction")
          + Option ("nperms_nonstationary", "the number of permutations used when precomputing the empirical statistic image for nonstationary correction (Default: " + str(DEFAULT_NUMBER_PERMUTATIONS_NONSTATIONARITY) + ")")
            + Argument ("num").type_integer (1)
          + Option ("permutations_nonstationary", "manually define the permutations (relabelling) for computing the emprical statistic image for nonstationary correction. "
                                                  "The input should be a text file defining a m x n matrix, where each relabelling is defined as a column vector of size m, "
                                                  "and the number of columns, n, defines the number of permutations. Can be generated with the palm_quickperms function in PALM "
                                                  "(http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM) "
                                                  "Overrides the nperms_nonstationary option.")
            + Argument ("file").type_file_in();
        }


        return result;
      }



    }
  }
}

