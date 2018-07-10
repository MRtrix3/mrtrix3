/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef __math_stats_shuffle_h__
#define __math_stats_shuffle_h__

#include "app.h"
#include "progressbar.h"
#include "types.h"

#include "misc/bitset.h"

#include "math/stats/typedefs.h"


#define DEFAULT_NUMBER_SHUFFLES 5000
#define DEFAULT_NUMBER_SHUFFLES_NONSTATIONARITY 5000


namespace MR
{
  namespace Math
  {
    namespace Stats
    {



      // TODO Generic command-line options:
      // - Set nature of errors
      // - Set number of shuffles (actual & nonstationarity correction)
      // - Import permutations (actual & nonstationarity correction)
      // - Import sign-flips (actual & nonstationarity correction)
      // - (future) Set exchangeability blocks

      extern const char* error_types[];
      App::OptionGroup shuffle_options (const bool include_nonstationarity, const default_type default_skew = 1.0);



      class Shuffle
      { NOMEMALIGN
        public:
          size_t index;
          matrix_type data;
      };



      class Shuffler
      { NOMEMALIGN
        public:
          typedef vector<size_t> PermuteLabels;

          // TODO Consider alternative interface allowing class to be initialised without
          //   ever accessing command-line options

          Shuffler (const size_t num_subjects, bool is_nonstationarity, const std::string msg = "");

          // Don't store the full set of shuffling matrices;
          //   generate each as it is required, based on the more compressed representations
          bool operator() (Shuffle& output);

          size_t size() const { return nshuffles; }


        private:
          const size_t rows;
          vector<PermuteLabels> permutations;
          vector<BitSet> signflips;
          size_t nshuffles, counter;
          std::unique_ptr<ProgressBar> progress;


          // For generating unique permutations
          bool is_duplicate (const PermuteLabels&, const PermuteLabels&) const;
          bool is_duplicate (const PermuteLabels&) const;

          // Note that this function does not take into account grouping of subjects and therefore generated
          // permutations are not guaranteed to be unique wrt the computed test statistic.
          // Providing the number of subjects is large then the likelihood of generating duplicates is low.
          void generate_permutations (const size_t num_perms,
                                      const size_t num_subjects,
                                      const bool include_default);

          void load_permutations (const std::string& filename);

          // Similar functions required for sign-flipping
          bool is_duplicate (const BitSet&) const;
          void generate_signflips (const size_t num_signflips,
                                   const size_t num_subjects,
                                   const bool include_default);
          void load_signflips (const std::string& filename);

      };



    }
  }
}

#endif
