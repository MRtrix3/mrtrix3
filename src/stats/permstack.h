/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#ifndef __stats_permstack_h__
#define __stats_permstack_h__

#include <mutex>
#include <stdint.h>

#include "progressbar.h"
#include "types.h"
#include "math/stats/permutation.h"

namespace MR
{
  namespace Stats
  {
    namespace PermTest
    {


      class Permutation
      { MEMALIGN (Permutation)
        public:
          size_t index;
          vector<size_t> data;
      };


      class PermutationStack 
      { MEMALIGN (PermutationStack)
        public:
          PermutationStack (const size_t num_permutations, const size_t num_samples, const std::string msg, const bool include_default = true);

          PermutationStack (vector <vector<size_t> >& permutations, const std::string msg);

          bool operator() (Permutation&);

          const vector<size_t>& operator[] (size_t index) const {
            return permutations[index];
          }

          const size_t num_permutations;

        protected:
          vector< vector<size_t> > permutations;
          size_t counter;
          ProgressBar progress;
      };




    }
  }
}

#endif
