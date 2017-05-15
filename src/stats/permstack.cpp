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


#include "stats/permstack.h"

namespace MR
{
  namespace Stats
  {
    namespace PermTest
    {



      PermutationStack::PermutationStack (const size_t num_permutations, const size_t num_samples, const std::string msg, const bool include_default) :
          num_permutations (num_permutations),
          counter (0),
          progress (msg, num_permutations)
      {
        Math::Stats::Permutation::generate (num_permutations, num_samples, permutations, include_default);
      }

      PermutationStack::PermutationStack (vector <vector<size_t> >& permutations, const std::string msg) :
          num_permutations (permutations.size()),
          permutations (permutations),
          counter (0),
          progress (msg, permutations.size()) { }



      bool PermutationStack::operator() (Permutation& out)
      {
        if (counter < num_permutations) {
          out.index = counter;
          out.data = permutations[counter++];
          ++progress;
          return true;
        } else {
          out.index = num_permutations;
          out.data.clear();
          return false;
        }
      }



    }
  }
}
