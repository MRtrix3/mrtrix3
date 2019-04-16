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


#include "math/stats/shuffle.h"

#include <algorithm>
#include <random>

#include "math/factorial.h"
#include "math/math.h"

namespace MR
{
  namespace Math
  {
    namespace Stats
    {



      const char* error_types[] = { "ee", "ise", "both", nullptr };


      App::OptionGroup shuffle_options (const bool include_nonstationarity, const default_type default_skew)
      {
        using namespace App;

        OptionGroup result = OptionGroup ("Options relating to shuffling of data for nonparametric statistical inference")

        + Option ("notest", "don't perform statistical inference; only output population statistics (effect size, stdev etc)")

        + Option ("errors", "specify nature of errors for shuffling; options are: " + join(error_types, ",") + " (default: ee)")
          + Argument ("spec").type_choice (error_types)

        + Option ("nshuffles", "the number of shuffles (default: " + str(DEFAULT_NUMBER_SHUFFLES) + ")")
          + Argument ("number").type_integer (1)

        + Option ("permutations", "manually define the permutations (relabelling). The input should be a text file defining a m x n matrix, "
                                  "where each relabelling is defined as a column vector of size m, and the number of columns, n, defines "
                                  "the number of permutations. Can be generated with the palm_quickperms function in PALM (http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM). "
                                  "Overrides the -nshuffles option.")
          + Argument ("file").type_file_in();

        if (include_nonstationarity) {

          result
          + Option ("nonstationarity", "perform non-stationarity correction")

          + Option ("skew_nonstationarity", "specify the skew parameter for empirical statistic calculation (default for this command is " + str(default_skew) + ")")
            + Argument ("value").type_float (0.0)

          + Option ("nshuffles_nonstationarity", "the number of shuffles to use when precomputing the empirical statistic image for non-stationarity correction (default: " + str(DEFAULT_NUMBER_SHUFFLES_NONSTATIONARITY) + ")")
            + Argument ("number").type_integer (1)

          + Option ("permutations_nonstationarity", "manually define the permutations (relabelling) for computing the emprical statistics for non-stationarity correction. "
                                                    "The input should be a text file defining a m x n matrix, where each relabelling is defined as a column vector of size m, "
                                                    "and the number of columns, n, defines the number of permutations. Can be generated with the palm_quickperms function in PALM "
                                                    "(http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM) "
                                                    "Overrides the -nshuffles_nonstationarity option.")
            + Argument ("file").type_file_in();

        }

        return result;
      }




      Shuffler::Shuffler (const size_t num_rows, const bool is_nonstationarity, const std::string msg) :
          rows (num_rows),
          nshuffles (is_nonstationarity ? DEFAULT_NUMBER_SHUFFLES_NONSTATIONARITY : DEFAULT_NUMBER_SHUFFLES),
          counter (0)
      {
        using namespace App;
        auto opt = get_options ("errors");
        error_t error_types = error_t::EE;
        if (opt.size()) {
          switch (int(opt[0][0])) {
            case 0: error_types = error_t::EE; break;
            case 1: error_types = error_t::ISE; break;
            case 2: error_types = error_t::BOTH; break;
          }
        }

        bool nshuffles_explicit = false;
        opt = get_options (is_nonstationarity ? "nshuffles_nonstationarity" : "nshuffles");
        if (opt.size()) {
          nshuffles = opt[0][0];
          nshuffles_explicit = true;
        }

        opt = get_options (is_nonstationarity ? "permutations_nonstationarity" : "permutations");
        if (opt.size()) {
          if (error_types == error_t::EE || error_types == error_t::BOTH) {
            load_permutations (opt[0][0]);
            if (permutations[0].size() != rows)
              throw Exception ("Number of entries per shuffle in file \"" + std::string (opt[0][0]) + "\" does not match number of rows in design matrix (" + str(rows) + ")");
            if (nshuffles_explicit && nshuffles != permutations.size())
              throw Exception ("Number of shuffles explicitly requested (" + str(nshuffles) + ") does not match number of shuffles in file \"" + std::string (opt[0][0]) + "\" (" + str(permutations.size()) + ")");
            nshuffles = permutations.size();
          } else {
            throw Exception ("Cannot manually provide permutations if errors are not exchangeable");
          }
        }

        initialise (error_types, nshuffles_explicit, is_nonstationarity);

        if (msg.size())
          progress.reset (new ProgressBar (msg, nshuffles));
      }




      Shuffler::Shuffler (const size_t num_rows,
                          const size_t num_shuffles,
                          const error_t error_types,
                          const bool is_nonstationarity,
                          const std::string msg) :
          rows (num_rows),
          nshuffles (num_shuffles)
      {
        initialise (error_types, true, is_nonstationarity);
        if (msg.size())
          progress.reset (new ProgressBar (msg, nshuffles));
      }





      bool Shuffler::operator() (Shuffle& output)
      {
        output.index = counter;
        if (counter >= nshuffles) {
          if (progress)
            progress.reset (nullptr);
          output.data.resize (0, 0);
          return false;
        }
        if (permutations.size()) {
          output.data = matrix_type::Zero (rows, rows);
          for (size_t i = 0; i != rows; ++i)
            output.data (i, permutations[counter][i]) = 1.0;
        } else {
          output.data = matrix_type::Identity (rows, rows);
        }
        if (signflips.size()) {
          for (size_t r = 0; r != rows; ++r) {
            if (signflips[counter][r]) {
              for (size_t c = 0; c != rows; ++c) {
                if (output.data (r, c))
                  output.data (r, c) *= -1.0;
              }
            }
          }
        }
        ++counter;
        if (progress)
          ++(*progress);
        return true;
      }






      void Shuffler::initialise (const error_t error_types,
                                 const bool nshuffles_explicit,
                                 const bool is_nonstationarity)
      {
        const bool ee = (error_types == error_t::EE || error_types == error_t::BOTH);
        const bool ise = (error_types == error_t::ISE || error_types == error_t::BOTH);

        const size_t max_num_permutations = factorial (rows);
        const size_t max_num_signflips = (rows >= 8*sizeof(size_t)) ?
                                         (std::numeric_limits<size_t>::max()) :
                                         ((size_t(1) << rows));
        size_t max_shuffles;
        if (ee) {
          if (ise) {
            max_shuffles = max_num_permutations * max_num_signflips;
            if (max_shuffles / max_num_signflips != max_num_permutations)
              max_shuffles = std::numeric_limits<size_t>::max();
          } else {
            max_shuffles = max_num_permutations;
          }
        } else {
          max_shuffles = max_num_signflips;
        }

        if (max_shuffles < nshuffles) {
          if (nshuffles_explicit) {
            WARN ("User requested " + str(nshuffles) + " shuffles for " +
                  (is_nonstationarity ? "non-stationarity correction" : "null distribution generation") +
                  ", but only " + str(max_shuffles) + " unique shuffles can be generated; "
                  "this will restrict the minimum achievable p-value to " + str(1.0/max_shuffles));
          } else {
            WARN ("Only " + str(max_shuffles) + " unique shuffles can be generated, which is less than the default number of " +
                  str(nshuffles) + " for " + (is_nonstationarity ? "non-stationarity correction" : "null distribution generation"));
          }
          nshuffles = max_shuffles;
        }

        // Need special handling of cases where both ee and ise are used
        // - If forced to use all shuffles, need to:
        //   - Generate all permutations, but duplicate each according to the number of signflips
        //   - Generate all signflips, but duplicate each according to the number of permutations
        //   - Interleave one of the two of them, so that every combination of paired permutation-signflip is unique
        // - If using a fixed number of shuffles:
        //   - If fixed number is less than the maximum number of permutations, generate that number randomly
        //   - If fixed number is equal to the maximum number of permutations, generate the full set
        //   - If fixed number is greater than the maximum number of permutations, generate that number randomly,
        //     while disabling detection of duplicates
        //   - Repeat the three steps above for signflips

        if (ee && !permutations.size()) {
          if (ise) {
            if (nshuffles == max_shuffles) {
              generate_all_permutations (rows);
              assert (permutations.size() == max_num_permutations);
              vector<PermuteLabels> duplicated_permutations;
              for (const auto& p : permutations) {
                for (size_t i = 0; i != max_num_signflips; ++i)
                  duplicated_permutations.push_back (p);
              }
              std::swap (permutations, duplicated_permutations);
              assert (permutations.size() == max_shuffles);
            } else if (nshuffles == max_num_permutations) {
              generate_all_permutations (rows);
              assert (permutations.size() == max_num_permutations);
            } else {
              // - Only include the default shuffling if this is the actual permutation testing;
              //   if we're doing nonstationarity correction, don't include the default
              // - Permit duplicates (specifically of permutations only) if an adequate number cannot be generated
              generate_random_permutations (nshuffles, rows, !is_nonstationarity, nshuffles > max_num_permutations);
            }
          } else if (nshuffles < max_shuffles) {
            generate_random_permutations (nshuffles, rows, !is_nonstationarity, false);
          } else {
            generate_all_permutations (rows);
            assert (permutations.size() == max_shuffles);
          }
        }

        if (ise) {
          if (ee) {
            if (nshuffles == max_shuffles) {
              generate_all_signflips (rows);
              assert (signflips.size() == max_num_signflips);
              vector<BitSet> duplicated_signflips;
              for (size_t i = 0; i != max_num_permutations; ++i)
                duplicated_signflips.insert (duplicated_signflips.end(), signflips.begin(), signflips.end());
              std::swap (signflips, duplicated_signflips);
              assert (signflips.size() == max_shuffles);
            } else if (nshuffles == max_num_signflips) {
              generate_all_signflips (rows);
              assert (signflips.size() == max_num_signflips);
            } else {
              generate_random_signflips (nshuffles, rows, !is_nonstationarity, nshuffles > max_num_signflips);
            }
          } else if (nshuffles < max_shuffles) {
            generate_random_signflips (nshuffles, rows, !is_nonstationarity, false);
          } else {
            generate_all_signflips (rows);
            assert (signflips.size() == max_shuffles);
          }
        }

        nshuffles = std::min (nshuffles, max_shuffles);
      }





      bool Shuffler::is_duplicate (const PermuteLabels& v1, const PermuteLabels& v2) const
      {
        assert (v1.size() == v2.size());
        for (size_t i = 0; i < v1.size(); i++) {
          if (v1[i] != v2[i])
            return false;
        }
        return true;
      }



      bool Shuffler::is_duplicate (const PermuteLabels& perm) const
      {
        for (const auto& p : permutations) {
          if (is_duplicate (perm, p))
            return true;
        }
        return false;
      }



      void Shuffler::generate_random_permutations (const size_t num_perms,
                                                   const size_t num_rows,
                                                   const bool include_default,
                                                   const bool permit_duplicates)
      {
        permutations.clear();
        permutations.reserve (num_perms);

        PermuteLabels default_labelling (num_rows);
        for (size_t i = 0; i < num_rows; ++i)
          default_labelling[i] = i;

        size_t p = 0;
        if (include_default) {
          permutations.push_back (default_labelling);
          ++p;
        }
        for (; p != num_perms; ++p) {
          PermuteLabels permuted_labelling (default_labelling);
          do {
            std::random_shuffle (permuted_labelling.begin(), permuted_labelling.end());
          } while (!permit_duplicates && is_duplicate (permuted_labelling));
          permutations.push_back (permuted_labelling);
        }
      }



      void Shuffler::generate_all_permutations (const size_t num_rows)
      {
        permutations.clear();
        permutations.reserve (factorial (num_rows));
        PermuteLabels temp (num_rows);
        for (size_t i = 0; i < num_rows; ++i)
          temp[i] = i;
        permutations.push_back (temp);
        while (std::next_permutation (temp.begin(), temp.end()))
          permutations.push_back (temp);
      }



      void Shuffler::load_permutations (const std::string& filename)
      {
        vector<vector<size_t> > temp = load_matrix_2D_vector<size_t> (filename);
        if (!temp.size())
          throw Exception ("no data found in permutations file: " + str(filename));

        const size_t min_value = *std::min_element (std::begin (temp[0]), std::end (temp[0]));
        if (min_value > 1)
          throw Exception ("indices for relabelling in permutations file must start from either 0 or 1");

        // TODO Support transposed permutations
        permutations.assign (temp[0].size(), PermuteLabels (temp.size()));
        for (size_t i = 0; i != temp[0].size(); i++) {
          for (size_t j = 0; j != temp.size(); j++)
            permutations[i][j] = temp[j][i] - min_value;
        }
      }




      bool Shuffler::is_duplicate (const BitSet& sign) const
      {
        for (const auto& s : signflips) {
          if (sign == s)
            return true;
        }
        return false;
      }



      void Shuffler::generate_random_signflips (const size_t num_signflips,
                                                const size_t num_rows,
                                                const bool include_default,
                                                const bool permit_duplicates)
      {
        signflips.clear();
        signflips.reserve (num_signflips);
        size_t s = 0;
        if (include_default) {
          BitSet default_labelling (num_rows, false);
          signflips.push_back (default_labelling);
          ++s;
        }
        std::random_device rd;
        std::mt19937 generator (rd());
        std::uniform_int_distribution<> distribution (0, 1);
        for (; s != num_signflips; ++s) {
          BitSet rows_to_flip (num_rows);
          do {
            // TODO Should be a faster mechanism for generating / storing random bits
            for (size_t index = 0; index != num_rows; ++index)
              rows_to_flip[index] = distribution (generator);
          } while (!permit_duplicates && is_duplicate (rows_to_flip));
          signflips.push_back (rows_to_flip);
        }
      }



      void Shuffler::generate_all_signflips (const size_t num_rows)
      {
        signflips.clear();
        signflips.reserve (size_t(1) << num_rows);
        BitSet temp (num_rows, false);
        signflips.push_back (temp);
        while (!temp.full()) {
          size_t last_zero_index;
          for (last_zero_index = num_rows - 1; temp[last_zero_index]; --last_zero_index);
          temp[last_zero_index] = true;
          for (size_t indices_zeroed = last_zero_index + 1; indices_zeroed != num_rows; ++indices_zeroed)
            temp[indices_zeroed] = false;
          signflips.push_back (temp);
        }
      }



    }
  }
}
