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



      PreProcessor::PreProcessor (const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                                  const std::shared_ptr<EnhancerBase> enhancer,
                                  matrix_type& global_enhanced_sum,
                                  vector<vector<size_t>>& global_enhanced_count) :
          stats_calculator (stats_calculator),
          enhancer (enhancer),
          global_enhanced_sum (global_enhanced_sum),
          global_enhanced_count (global_enhanced_count),
          enhanced_sum (vector_type::Zero (global_enhanced_sum.size())),
          enhanced_count (stats_calculator->num_outputs(), vector<size_t> (stats_calculator->num_elements(), 0)),
          stats (global_enhanced_sum.rows(), global_enhanced_sum.cols()),
          enhanced_stats (global_enhanced_sum.rows(), global_enhanced_sum.cols()),
          mutex (new std::mutex())
      {
        assert (stats_calculator);
        assert (enhancer);
      }



      PreProcessor::~PreProcessor ()
      {
        std::lock_guard<std::mutex> lock (*mutex);
        global_enhanced_sum.array() += enhanced_sum.array();
        for (ssize_t row = 0; row != global_enhanced_sum.rows(); ++row) {
          for (ssize_t col = 0; col != global_enhanced_sum.cols(); ++col)
            global_enhanced_count[row][col] += enhanced_count[row][col];
        }
      }



      bool PreProcessor::operator() (const Permutation& permutation)
      {
        if (permutation.data.empty())
          return false;
        (*stats_calculator) (permutation.data, stats);
        (*enhancer) (stats, enhanced_stats);
        for (ssize_t c = 0; c != enhanced_stats.rows(); ++c) {
          for (ssize_t i = 0; i < enhanced_stats.cols(); ++i) {
            if (enhanced_stats(c, i) > 0.0) {
              enhanced_sum(c, i) += enhanced_stats(c, i);
              enhanced_count[c][i]++;
            }
          }
        }
        return true;
      }







      Processor::Processor (const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                            const std::shared_ptr<EnhancerBase> enhancer,
                            const matrix_type& empirical_enhanced_statistics,
                            const matrix_type& default_enhanced_statistics,
                            matrix_type& perm_dist,
                            vector<vector<size_t>>& global_uncorrected_pvalue_counter) :
          stats_calculator (stats_calculator),
          enhancer (enhancer),
          empirical_enhanced_statistics (empirical_enhanced_statistics),
          default_enhanced_statistics (default_enhanced_statistics),
          statistics (stats_calculator->num_outputs(), stats_calculator->num_elements()),
          enhanced_statistics (stats_calculator->num_outputs(), stats_calculator->num_elements()),
          uncorrected_pvalue_counter (stats_calculator->num_outputs(), vector<size_t> (stats_calculator->num_elements(), 0)),
          perm_dist (perm_dist),
          global_uncorrected_pvalue_counter (global_uncorrected_pvalue_counter),
          mutex (new std::mutex())
      {
        assert (stats_calculator);
      }



      Processor::~Processor ()
      {
        std::lock_guard<std::mutex> lock (*mutex);
        for (size_t row = 0; row != stats_calculator->num_outputs(); ++row) {
          for (size_t i = 0; i < stats_calculator->num_elements(); ++i)
            global_uncorrected_pvalue_counter[row][i] += uncorrected_pvalue_counter[row][i];
        }
      }



      bool Processor::operator() (const Permutation& permutation)
      {
        (*stats_calculator) (permutation.data, statistics);
        if (enhancer)
          (*enhancer) (statistics, enhanced_statistics);
        else
          enhanced_statistics = statistics;

        if (empirical_enhanced_statistics.size())
          enhanced_statistics.array() /= empirical_enhanced_statistics.array();

        perm_dist.col(permutation.index) = enhanced_statistics.rowwise().maxCoeff();

        for (ssize_t row = 0; row != enhanced_statistics.rows(); ++row) {
          for (ssize_t i = 0; i != enhanced_statistics.cols(); ++i) {
            if (default_enhanced_statistics(row, i) > enhanced_statistics(row, i))
              uncorrected_pvalue_counter[row][i]++;
          }
        }

        return true;
      }







      void precompute_empirical_stat (const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                                      const std::shared_ptr<EnhancerBase> enhancer,
                                      PermutationStack& perm_stack, matrix_type& empirical_statistic)
      {
        vector<vector<size_t>> global_enhanced_count (empirical_statistic.rows(), vector<size_t> (empirical_statistic.cols(), 0));
        {
          PreProcessor preprocessor (stats_calculator, enhancer, empirical_statistic, global_enhanced_count);
          Thread::run_queue (perm_stack, Permutation(), Thread::multi (preprocessor));
        }
        for (ssize_t row = 0; row != empirical_statistic.rows(); ++row) {
          for (ssize_t i = 0; i != empirical_statistic.cols(); ++i) {
            if (global_enhanced_count[row][i] > 0.0)
              empirical_statistic(row, i) /= static_cast<default_type> (global_enhanced_count[row][i]);
          }
        }
      }




      void precompute_default_permutation (const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                                           const std::shared_ptr<EnhancerBase> enhancer,
                                           const matrix_type& empirical_enhanced_statistic,
                                           matrix_type& default_enhanced_statistics,
                                           matrix_type& default_statistics)
      {
        vector<size_t> default_labelling (stats_calculator->num_subjects());
        for (size_t i = 0; i < default_labelling.size(); ++i)
          default_labelling[i] = i;

        (*stats_calculator) (default_labelling, default_statistics);

        if (enhancer)
          (*enhancer) (default_statistics, default_enhanced_statistics);
        else
          default_enhanced_statistics = default_statistics;

        if (empirical_enhanced_statistic.size())
          default_enhanced_statistics.array() /= empirical_enhanced_statistic.array();
      }




      void run_permutations (PermutationStack& perm_stack,
                             const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                             const std::shared_ptr<EnhancerBase> enhancer,
                             const matrix_type& empirical_enhanced_statistic,
                             const matrix_type& default_enhanced_statistics,
                             matrix_type& perm_dist,
                             matrix_type& uncorrected_pvalues)
      {
        vector<vector<size_t>> global_uncorrected_pvalue_count (stats_calculator->num_outputs(), vector<size_t> (stats_calculator->num_elements(), 0));
        {
          Processor processor (stats_calculator, enhancer,
                               empirical_enhanced_statistic,
                               default_enhanced_statistics,
                               perm_dist,
                               global_uncorrected_pvalue_count);
          Thread::run_queue (perm_stack, Permutation(), Thread::multi (processor));
        }

        for (size_t row = 0; row != stats_calculator->num_outputs(); ++row) {
          for (size_t i = 0; i < stats_calculator->num_elements(); ++i)
            uncorrected_pvalues(row, i) = global_uncorrected_pvalue_count[row][i] / default_type(perm_stack.num_permutations);
        }
      }




      void run_permutations (const vector<vector<size_t>>& permutations,
                             const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                             const std::shared_ptr<EnhancerBase> enhancer,
                             const matrix_type& empirical_enhanced_statistic,
                             const matrix_type& default_enhanced_statistics,
                             matrix_type& perm_dist,
                             matrix_type& uncorrected_pvalues)
      {
        PermutationStack perm_stack (permutations, "running " + str(permutations.size()) + " permutations");

        run_permutations (perm_stack, stats_calculator, enhancer, empirical_enhanced_statistic,
                          default_enhanced_statistics, perm_dist, uncorrected_pvalues);
      }




      void run_permutations (const size_t num_permutations,
                             const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                             const std::shared_ptr<EnhancerBase> enhancer,
                             const matrix_type& empirical_enhanced_statistic,
                             const matrix_type& default_enhanced_statistics,
                             matrix_type& perm_dist,
                             matrix_type& uncorrected_pvalues)
      {
        PermutationStack perm_stack (num_permutations, stats_calculator->num_subjects(), "running " + str(num_permutations) + " permutations");

        run_permutations (perm_stack, stats_calculator, enhancer, empirical_enhanced_statistic,
                          default_enhanced_statistics, perm_dist, uncorrected_pvalues);
      }




    }
  }
}

