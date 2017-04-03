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
                                  vector_type& global_enhanced_sum,
                                  vector<size_t>& global_enhanced_count) :
          stats_calculator (stats_calculator),
          enhancer (enhancer), global_enhanced_sum (global_enhanced_sum),
          global_enhanced_count (global_enhanced_count), enhanced_sum (vector_type::Zero (global_enhanced_sum.size())),
          enhanced_count (global_enhanced_sum.size(), 0.0), stats (global_enhanced_sum.size()),
          enhanced_stats (global_enhanced_sum.size()), mutex (new std::mutex())
      {
        assert (stats_calculator);
        assert (enhancer);
      }



      PreProcessor::~PreProcessor ()
      {
        std::lock_guard<std::mutex> lock (*mutex);
        for (ssize_t i = 0; i < global_enhanced_sum.size(); ++i) {
          global_enhanced_sum[i] += enhanced_sum[i];
          global_enhanced_count[i] += enhanced_count[i];
        }
      }



      bool PreProcessor::operator() (const Permutation& permutation)
      {
        (*stats_calculator) (permutation.data, stats);
        (*enhancer) (stats, enhanced_stats);
        for (ssize_t i = 0; i < enhanced_stats.size(); ++i) {
          if (enhanced_stats[i] > 0.0) {
            enhanced_sum[i] += enhanced_stats[i];
            enhanced_count[i]++;
          }
        }
        return true;
      }







      Processor::Processor (const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                            const std::shared_ptr<EnhancerBase> enhancer,
                            const vector_type& empirical_enhanced_statistics,
                            const vector_type& default_enhanced_statistics,
                            const std::shared_ptr<vector_type> default_enhanced_statistics_neg,
                            vector_type& perm_dist_pos,
                            std::shared_ptr<vector_type> perm_dist_neg,
                            vector<size_t>& global_uncorrected_pvalue_counter,
                            std::shared_ptr< vector<size_t> > global_uncorrected_pvalue_counter_neg) :
          stats_calculator (stats_calculator),
          enhancer (enhancer), empirical_enhanced_statistics (empirical_enhanced_statistics),
          default_enhanced_statistics (default_enhanced_statistics), default_enhanced_statistics_neg (default_enhanced_statistics_neg),
          statistics (stats_calculator->num_elements()), enhanced_statistics (stats_calculator->num_elements()),
          uncorrected_pvalue_counter (stats_calculator->num_elements(), 0),
          perm_dist_pos (perm_dist_pos), perm_dist_neg (perm_dist_neg),
          global_uncorrected_pvalue_counter (global_uncorrected_pvalue_counter),
          global_uncorrected_pvalue_counter_neg (global_uncorrected_pvalue_counter_neg),
          mutex (new std::mutex())
      {
        assert (stats_calculator);
        if (global_uncorrected_pvalue_counter_neg)
          uncorrected_pvalue_counter_neg.reset (new vector<size_t>(stats_calculator->num_elements(), 0));
      }



      Processor::~Processor ()
      {
        std::lock_guard<std::mutex> lock (*mutex);
        for (size_t i = 0; i < stats_calculator->num_elements(); ++i) {
          global_uncorrected_pvalue_counter[i] += uncorrected_pvalue_counter[i];
          if (global_uncorrected_pvalue_counter_neg)
            (*global_uncorrected_pvalue_counter_neg)[i] = (*uncorrected_pvalue_counter_neg)[i];
        }
      }



      bool Processor::operator() (const Permutation& permutation)
      {
        (*stats_calculator) (permutation.data, statistics);
        if (enhancer) {
          perm_dist_pos[permutation.index] = (*enhancer) (statistics, enhanced_statistics);
        } else {
          enhanced_statistics = statistics;
          perm_dist_pos[permutation.index] = enhanced_statistics.maxCoeff();
        }

        if (empirical_enhanced_statistics.size()) {
          perm_dist_pos[permutation.index] = 0.0;
          for (ssize_t i = 0; i < enhanced_statistics.size(); ++i) {
            enhanced_statistics[i] /= empirical_enhanced_statistics[i];
            perm_dist_pos[permutation.index] = std::max(perm_dist_pos[permutation.index], enhanced_statistics[i]);
          }
        }

        for (ssize_t i = 0; i < enhanced_statistics.size(); ++i) {
          if (default_enhanced_statistics[i] > enhanced_statistics[i])
            uncorrected_pvalue_counter[i]++;
        }

        // Compute the opposite contrast
        if (perm_dist_neg) {
          statistics = -statistics;

          (*perm_dist_neg)[permutation.index] = (*enhancer) (statistics, enhanced_statistics);

          if (empirical_enhanced_statistics.size()) {
            (*perm_dist_neg)[permutation.index] = 0.0;
            for (ssize_t i = 0; i < enhanced_statistics.size(); ++i) {
              enhanced_statistics[i] /= empirical_enhanced_statistics[i];
              (*perm_dist_neg)[permutation.index] = std::max ((*perm_dist_neg)[permutation.index], enhanced_statistics[i]);
            }
          }

          for (ssize_t i = 0; i < enhanced_statistics.size(); ++i) {
            if ((*default_enhanced_statistics_neg)[i] > enhanced_statistics[i])
              (*uncorrected_pvalue_counter_neg)[i]++;
          }
        }
        return true;
      }







      void precompute_empirical_stat (const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                                      const std::shared_ptr<EnhancerBase> enhancer,
                                      PermutationStack& perm_stack, vector_type& empirical_statistic)
      {
        vector<size_t> global_enhanced_count (empirical_statistic.size(), 0);
        {
          PreProcessor preprocessor (stats_calculator, enhancer, empirical_statistic, global_enhanced_count);
          Thread::run_queue (perm_stack, Permutation(), Thread::multi (preprocessor));
        }
        for (ssize_t i = 0; i < empirical_statistic.size(); ++i) {
          if (global_enhanced_count[i] > 0)
            empirical_statistic[i] /= static_cast<default_type> (global_enhanced_count[i]);
        }
      }




      void precompute_default_permutation (const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                                           const std::shared_ptr<EnhancerBase> enhancer,
                                           const vector_type& empirical_enhanced_statistic,
                                           vector_type& default_enhanced_statistics,
                                           std::shared_ptr<vector_type> default_enhanced_statistics_neg,
                                           vector_type& default_statistics)
      {
        vector<size_t> default_labelling (stats_calculator->num_subjects());
        for (size_t i = 0; i < default_labelling.size(); ++i)
          default_labelling[i] = i;
        (*stats_calculator) (default_labelling, default_statistics);
        (*enhancer) (default_statistics, default_enhanced_statistics);

        if (empirical_enhanced_statistic.size())
          default_enhanced_statistics /= empirical_enhanced_statistic;

        // Compute the opposite contrast
        if (default_enhanced_statistics_neg) {
          default_statistics = -default_statistics;

          (*enhancer) (default_statistics, *default_enhanced_statistics_neg);

          if (empirical_enhanced_statistic.size())
            (*default_enhanced_statistics_neg) /= empirical_enhanced_statistic;

          // revert default_statistics to positive contrast for output
          default_statistics = -default_statistics;
        }
      }




      void run_permutations (PermutationStack& perm_stack,
                             const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                             const std::shared_ptr<EnhancerBase> enhancer,
                             const vector_type& empirical_enhanced_statistic,
                             const vector_type& default_enhanced_statistics,
                             const std::shared_ptr<vector_type> default_enhanced_statistics_neg,
                             vector_type& perm_dist_pos,
                             std::shared_ptr<vector_type> perm_dist_neg,
                             vector_type& uncorrected_pvalues,
                             std::shared_ptr<vector_type> uncorrected_pvalues_neg)
      {
        vector<size_t> global_uncorrected_pvalue_count (stats_calculator->num_elements(), 0);
        std::shared_ptr< vector<size_t> > global_uncorrected_pvalue_count_neg;
        if (perm_dist_neg)
          global_uncorrected_pvalue_count_neg.reset (new vector<size_t> (stats_calculator->num_elements(), 0));

        {
          Processor processor (stats_calculator, enhancer,
                               empirical_enhanced_statistic,
                               default_enhanced_statistics, default_enhanced_statistics_neg,
                               perm_dist_pos, perm_dist_neg,
                               global_uncorrected_pvalue_count, global_uncorrected_pvalue_count_neg);
          Thread::run_queue (perm_stack, Permutation(), Thread::multi (processor));
        }

        for (size_t i = 0; i < stats_calculator->num_elements(); ++i) {
          uncorrected_pvalues[i] = global_uncorrected_pvalue_count[i] / default_type(perm_stack.num_permutations);
          if (perm_dist_neg)
            (*uncorrected_pvalues_neg)[i] = (*global_uncorrected_pvalue_count_neg)[i] / default_type(perm_stack.num_permutations);
        }
      }




      void run_permutations (const vector<vector<size_t>>& permutations,
                             const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                             const std::shared_ptr<EnhancerBase> enhancer,
                             const vector_type& empirical_enhanced_statistic,
                             const vector_type& default_enhanced_statistics,
                             const std::shared_ptr<vector_type> default_enhanced_statistics_neg,
                             vector_type& perm_dist_pos,
                             std::shared_ptr<vector_type> perm_dist_neg,
                             vector_type& uncorrected_pvalues,
                             std::shared_ptr<vector_type> uncorrected_pvalues_neg)
      {
        PermutationStack perm_stack (permutations, "running " + str(permutations.size()) + " permutations");

        run_permutations (perm_stack, stats_calculator, enhancer, empirical_enhanced_statistic, default_enhanced_statistics, default_enhanced_statistics_neg,
                          perm_dist_pos, perm_dist_neg, uncorrected_pvalues, uncorrected_pvalues_neg);
      }




      void run_permutations (const size_t num_permutations,
                             const std::shared_ptr<Math::Stats::GLMTestBase> stats_calculator,
                             const std::shared_ptr<EnhancerBase> enhancer,
                             const vector_type& empirical_enhanced_statistic,
                             const vector_type& default_enhanced_statistics,
                             const std::shared_ptr<vector_type> default_enhanced_statistics_neg,
                             vector_type& perm_dist_pos,
                             std::shared_ptr<vector_type> perm_dist_neg,
                             vector_type& uncorrected_pvalues,
                             std::shared_ptr<vector_type> uncorrected_pvalues_neg)
      {
        PermutationStack perm_stack (num_permutations, stats_calculator->num_subjects(), "running " + str(num_permutations) + " permutations");

        run_permutations (perm_stack, stats_calculator, enhancer, empirical_enhanced_statistic, default_enhanced_statistics, default_enhanced_statistics_neg,
                          perm_dist_pos, perm_dist_neg, uncorrected_pvalues, uncorrected_pvalues_neg);
      }




    }
  }
}

