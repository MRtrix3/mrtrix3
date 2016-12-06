/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
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
        +   Argument ("num").type_integer (1);

        if (include_nonstationarity) {
          result
          + Option ("nonstationary", "perform non-stationarity correction")
          + Option ("nperms_nonstationary", "the number of permutations used when precomputing the empirical statistic image for nonstationary correction (Default: " + str(DEFAULT_NUMBER_PERMUTATIONS_NONSTATIONARITY) + ")")
          +   Argument ("num").type_integer (1);
        }

        return result;
      }



    }
  }
}

