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

#include "stats/permstack.h"

namespace MR
{
  namespace Stats
  {
    namespace PermTest
    {



      PermutationStack::PermutationStack (const size_t num_permutations, const size_t num_samples, const std::string msg, const bool include_default) :
          num_permutations (num_permutations),
          current_permutation (0),
          progress (msg, num_permutations)
      {
        Math::Stats::Permutation::generate (num_permutations, num_samples, permutations, include_default);
      }



      size_t PermutationStack::next()
      {
        std::lock_guard<std::mutex> lock (permutation_mutex);
        size_t index = current_permutation++;
        if (index < permutations.size())
          ++progress;
        return index;
      }



    }
  }
}
