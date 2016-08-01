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

#ifndef __stats_permstack_h__
#define __stats_permstack_h__

#include <mutex>
#include <stdint.h>
#include <vector>

#include "progressbar.h"
#include "math/stats/permutation.h"

namespace MR
{
  namespace Stats
  {
    namespace PermTest
    {



      class PermutationStack {
        public:
          PermutationStack (const size_t num_permutations, const size_t num_samples, const std::string msg, const bool include_default = true);

          size_t next();

          const std::vector<size_t>& permutation (size_t index) const {
            return permutations[index];
          }

          const size_t num_permutations;

        protected:
          size_t current_permutation;
          ProgressBar progress;
          std::vector< std::vector<size_t> > permutations;
          std::mutex permutation_mutex;
      };




    }
  }
}

#endif
