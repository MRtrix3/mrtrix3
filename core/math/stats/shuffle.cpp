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

#include <random>

#include "math/math.h"

namespace MR
{
  namespace Math
  {
    namespace Stats
    {



      const char* error_types[] = { "ee", "ise", "both", nullptr };


      App::OptionGroup shuffle_options (const bool include_nonstationarity)
      {
        using namespace App;

        OptionGroup result = OptionGroup ("Options relating to shuffling of data for nonparametric statistical inference")

        + Option ("errors", "specify nature of errors for shuffling; options are: " + join(error_types, ",") + " (default: ee)")
          + Argument ("spec").type_choice (error_types)

        // TODO Find a better place for this
        //+ Option ("notest", "don't perform statistical inference; only output population statistics (effect size, stdev etc)")

        + Option ("nshuffles", "the number of shuffles (default: " + str(DEFAULT_NUMBER_SHUFFLES) + ")")
          + Argument ("number").type_integer (1)

        + Option ("permutations", "manually define the permutations (relabelling). The input should be a text file defining a m x n matrix, "
                                  "where each relabelling is defined as a column vector of size m, and the number of columns, n, defines "
                                  "the number of permutations. Can be generated with the palm_quickperms function in PALM (http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM). "
                                  "Overrides the -nshuffles option.")
          + Argument ("file").type_file_in()

        // TODO See what is available in PALM
        + Option ("signflips", "manually define the signflips")
          + Argument ("file").type_file_in();

        if (include_nonstationarity) {

          result
          + Option ("nonstationarity", "perform non-stationarity correction")

          + Option ("nshuffles_nonstationary", "the number of shuffles to use when precomputing the empirical statistic image for non-stationarity correction (default: " + str(DEFAULT_NUMBER_SHUFFLES_NONSTATIONARITY) + ")")
            + Argument ("number").type_integer (1)

          + Option ("permutations_nonstationarity", "manually define the permutations (relabelling) for computing the emprical statistics for non-stationarity correction. "
                                                    "The input should be a text file defining a m x n matrix, where each relabelling is defined as a column vector of size m, "
                                                    "and the number of columns, n, defines the number of permutations. Can be generated with the palm_quickperms function in PALM "
                                                    "(http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM) "
                                                    "Overrides the -nshuffles_nonstationarity option.")
            + Argument ("file").type_file_in()

          + Option ("signflips_nonstationarity", "manually define the signflips for computing the empirical statistics for non-stationarity correction")
            + Argument ("file").type_file_in();

        }

        return result;
      }



      Shuffler::Shuffler (const size_t num_subjects, const bool is_nonstationarity, const std::string msg) :
          rows (num_subjects),
          nshuffles (is_nonstationarity ? DEFAULT_NUMBER_SHUFFLES_NONSTATIONARITY : DEFAULT_NUMBER_SHUFFLES),
          counter (0)
      {
        using namespace App;
        auto opt = get_options ("errors");
        bool ee = true, ise = false;
        if (opt.size()) {
          switch (int(opt[0][0])) {
            case 0: ee = true;  ise = false; break;
            case 1: ee = false; ise = true;  break;
            case 2: ee = true;  ise = true;  break;
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
          if (ee) {
            load_permutations (opt[0][0]);
            if (permutations[0].size() != rows)
              throw Exception ("Number of entries per shuffle in file \"" + std::string (opt[0][0]) + "\" does not match number of rows in design matrix (" + str(rows) + ")");
            if (nshuffles_explicit && nshuffles != permutations.size())
              throw Exception ("Number of shuffles explicitly requested (" + str(nshuffles) + ") does not match number of shuffles in file \"" + std::string (opt[0][0]) + "\" (" + str(permutations.size()) + ")");
            nshuffles = permutations.size();
          } else {
            throw Exception ("Cannot manually provide permutations if errors are not exchangeable");
          }
        } else if (ee) {
          // Only include the default shuffling if this is the actual permutation testing;
          //   if we're doing nonstationarity correction, don't include the default
          generate_permutations (nshuffles, rows, !is_nonstationarity);
        }

        opt = get_options (is_nonstationarity ? "signflips_nonstationarity" : "signflips");
        if (opt.size()) {
          if (ise) {
            load_signflips (opt[0][0]);
            if (signflips[0].size() != rows)
              throw Exception ("Number of entries per shuffle in file \"" + std::string (opt[0][0]) + "\" does not match number of rows in design matrix (" + str(rows) + ")");
            if (nshuffles_explicit && nshuffles != signflips.size())
              throw Exception ("Number of shuffles explicitly requested (" + str(nshuffles) + ") does not match number of shuffles in file \"" + std::string (opt[0][0]) + "\" (" + str(signflips.size()) + ")");
            if (permutations.size() && signflips.size() != permutations.size())
              throw Exception ("Number of permutations (" + str(permutations.size()) + ") does not match number of signflips (" + str(signflips.size()) + ")");
            nshuffles = signflips.size();
          } else {
            throw Exception ("Cannot manually provide signflips if errors are not independent and symmetric");
          }
        } else if (ise) {
          generate_signflips (nshuffles, rows, !is_nonstationarity);
        }

        if (msg.size())
          progress.reset (new ProgressBar (msg, nshuffles));
      }




      bool Shuffler::operator() (Shuffle& output)
      {
        output.index = counter;
        if (counter + 1 >= nshuffles) {
          if (progress)
            progress.reset (nullptr);
          counter = nshuffles;
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
              //output.data.row (r) *= -1.0;
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
          for (const auto p : permutations) {
            if (is_duplicate (perm, p))
              return true;
          }
          return false;
        }



        void Shuffler::generate_permutations (const size_t num_perms,
                                              const size_t num_subjects,
                                              const bool include_default)
        {
          permutations.clear();
          permutations.reserve (num_perms);
          PermuteLabels default_labelling (num_subjects);
          for (size_t i = 0; i < num_subjects; ++i)
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
            } while (is_duplicate (permuted_labelling));
            permutations.push_back (permuted_labelling);
          }
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
          for (const auto s : signflips) {
            if (sign == s)
              return true;
          }
          return false;
        }



        void Shuffler::generate_signflips (const size_t num_signflips,
                                           const size_t num_subjects,
                                           const bool include_default)
        {
          signflips.clear();
          signflips.reserve (num_signflips);
          size_t s = 0;
          if (include_default) {
            BitSet default_labelling (num_subjects, false);
            signflips.push_back (default_labelling);
            ++s;
          }
          std::random_device rd;
          std::mt19937 generator (rd());
          std::uniform_int_distribution<> distribution (0, 1);
          for (; s != num_signflips; ++s) {
            BitSet rows_to_flip (num_subjects);
            do {
              // TODO Should be a faster mechanism for generating / storing random bits
              for (size_t index = 0; index != num_subjects; ++index)
                rows_to_flip[index] = distribution (generator);
            } while (is_duplicate (rows_to_flip));
            signflips.push_back (rows_to_flip);
          }
        }



        void Shuffler::load_signflips (const std::string&)
        {
          // TODO
          assert (0);
        }




    }
  }
}
