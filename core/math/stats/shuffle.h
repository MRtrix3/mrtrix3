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



      // Generic command-line options:
      // - Set nature of errors
      // - Set number of shuffles (actual & nonstationarity correction)
      // - Import permutations (actual & nonstationarity correction)
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
          enum class error_t { EE, ISE, BOTH };

          // First version reads command-line options in order to determine parameters prior to running initialise();
          //   second and third versions more-or-less call initialise() directly
          Shuffler (const size_t num_rows,
                    const bool is_nonstationarity,
                    const std::string msg = "");

          Shuffler (const size_t num_rows,
                    const size_t num_shuffles,
                    const error_t error_types,
                    const bool is_nonstationarity,
                    const std::string msg = "");

          Shuffler (const size_t num_rows,
                    const size_t num_shuffles,
                    const error_t error_types,
                    const bool is_nonstationarity,
                    const index_array_type& eb_within,
                    const index_array_type& eb_whole,
                    const std::string msg = "");

          // Don't store the full set of shuffling matrices;
          //   generate each as it is required, based on the more compressed representations
          bool operator() (Shuffle& output);

          size_t size() const { return nshuffles; }

          // Go back to the first permutation
          void reset();


        private:
          const size_t rows;
          vector<PermuteLabels> permutations;
          vector<BitSet> signflips;
          size_t nshuffles, counter;
          std::unique_ptr<ProgressBar> progress;


          void initialise (const error_t error_types,
                           const bool nshuffles_explicit,
                           const bool is_nonstationarity,
                           const index_array_type& eb_within,
                           const index_array_type& eb_whole);



          // For exchangeability blocks (either within or whole)
          index_array_type load_blocks (const std::string& filename, const bool equal_sizes);


          // For generating unique permutations
          bool is_duplicate (const PermuteLabels&, const PermuteLabels&) const;
          bool is_duplicate (const PermuteLabels&) const;

          // Note that this function does not take into account identical rows and therefore generated
          // permutations are not guaranteed to be unique wrt the computed test statistic.
          // Providing the number of rows is large then the likelihood of generating duplicates is low.
          void generate_random_permutations (const size_t num_perms,
                                             const size_t num_rows,
                                             const index_array_type& eb_within,
                                             const index_array_type& eb_whole,
                                             const bool include_default,
                                             const bool permit_duplicates);

          void generate_all_permutations (const size_t num_rows,
                                          const index_array_type& eb_within,
                                          const index_array_type& eb_whole);

          void load_permutations (const std::string& filename);

          // Similar functions required for sign-flipping
          bool is_duplicate (const BitSet&) const;

          void generate_random_signflips (const size_t num_signflips,
                                          const size_t num_rows,
                                          const index_array_type& blocks,
                                          const bool include_default,
                                          const bool permit_duplicates);

          void generate_all_signflips (const size_t num_rows,
                                       const index_array_type& blocks);


          vector<vector<size_t>> indices2blocks (const index_array_type&) const;

      };



    }
  }
}

#endif
