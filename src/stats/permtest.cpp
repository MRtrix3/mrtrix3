/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "stats/permtest.h"

namespace MR
{
  namespace Stats
  {
    namespace PermTest
    {



      PreProcessor::PreProcessor (const std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator,
                                  const std::shared_ptr<EnhancerBase> enhancer,
                                  const default_type skew,
                                  matrix_type& global_enhanced_sum,
                                  count_matrix_type& global_enhanced_count) :
          stats_calculator (stats_calculator),
          enhancer (enhancer),
          skew (skew),
          global_enhanced_sum (global_enhanced_sum),
          global_enhanced_count (global_enhanced_count),
          enhanced_sum (matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_outputs())),
          enhanced_count (count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_outputs())),
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
        global_enhanced_count += enhanced_count;
      }



      bool PreProcessor::operator() (const Math::Stats::Shuffle& shuffle)
      {
        if (!shuffle.data.rows())
          return false;
        (*stats_calculator) (shuffle.data, stats);
        (*enhancer) (stats, enhanced_stats);
        for (size_t c = 0; c != stats_calculator->num_outputs(); ++c) {
          for (size_t i = 0; i != stats_calculator->num_elements(); ++i) {
            if (enhanced_stats(i, c) > 0.0) {
              enhanced_sum(i, c) += std::pow (enhanced_stats(i, c), skew);
              enhanced_count(i, c)++;
            }
          }
        }
        return true;
      }







      Processor::Processor (const std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator,
                            const std::shared_ptr<EnhancerBase> enhancer,
                            const matrix_type& empirical_enhanced_statistics,
                            const matrix_type& default_enhanced_statistics,
                            matrix_type& perm_dist,
                            count_matrix_type& perm_dist_contributions,
                            count_matrix_type& global_uncorrected_pvalue_counter) :
          stats_calculator (stats_calculator),
          enhancer (enhancer),
          empirical_enhanced_statistics (empirical_enhanced_statistics),
          default_enhanced_statistics (default_enhanced_statistics),
          statistics (stats_calculator->num_elements(), stats_calculator->num_outputs()),
          enhanced_statistics (stats_calculator->num_elements(), stats_calculator->num_outputs()),
          perm_dist (perm_dist),
          global_perm_dist_contributions (perm_dist_contributions),
          perm_dist_contribution_counter (count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_outputs())),
          global_uncorrected_pvalue_counter (global_uncorrected_pvalue_counter),
          uncorrected_pvalue_counter (count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_outputs())),
          mutex (new std::mutex())
      {
        assert (stats_calculator);
      }



      Processor::~Processor ()
      {
        std::lock_guard<std::mutex> lock (*mutex);
        global_uncorrected_pvalue_counter += uncorrected_pvalue_counter;
        global_perm_dist_contributions += perm_dist_contribution_counter;
      }



      bool Processor::operator() (const Math::Stats::Shuffle& shuffle)
      {
        (*stats_calculator) (shuffle.data, statistics);
        if (enhancer)
          (*enhancer) (statistics, enhanced_statistics);
        else
          enhanced_statistics = statistics;

        if (empirical_enhanced_statistics.size())
          enhanced_statistics.array() /= empirical_enhanced_statistics.array();

        ssize_t max_index;
        for (ssize_t contrast = 0; contrast != enhanced_statistics.cols(); ++contrast) {
          perm_dist(shuffle.index, contrast) = enhanced_statistics.col (contrast).maxCoeff (&max_index);
          perm_dist_contribution_counter(max_index, contrast)++;
          for (ssize_t element = 0; element != enhanced_statistics.rows(); ++element) {
            if (default_enhanced_statistics(element, contrast) > enhanced_statistics(element, contrast))
              uncorrected_pvalue_counter(element, contrast)++;
          }
        }

        return true;
      }







      void precompute_empirical_stat (const std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator,
                                      const std::shared_ptr<EnhancerBase> enhancer,
                                      const default_type skew,
                                      matrix_type& empirical_statistic)
      {
        assert (stats_calculator);
        empirical_statistic = matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_outputs());
        count_matrix_type global_enhanced_count (count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_outputs()));
        {
          Math::Stats::Shuffler shuffler (stats_calculator->num_subjects(), true, "Pre-computing empirical statistic for non-stationarity correction");
          PreProcessor preprocessor (stats_calculator, enhancer, skew, empirical_statistic, global_enhanced_count);
          Thread::run_queue (shuffler, Math::Stats::Shuffle(), Thread::multi (preprocessor));
        }
        for (size_t contrast = 0; contrast != stats_calculator->num_outputs(); ++contrast) {
          for (size_t element = 0; element != stats_calculator->num_elements(); ++element) {
            if (global_enhanced_count(element, contrast) > 0)
              empirical_statistic(element, contrast) = std::pow (empirical_statistic(element, contrast) / static_cast<default_type> (global_enhanced_count(element, contrast)), 1.0/skew);
            else
              empirical_statistic(element, contrast) = std::numeric_limits<default_type>::infinity();
          }
        }
      }




      void precompute_default_permutation (const std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator,
                                           const std::shared_ptr<EnhancerBase> enhancer,
                                           const matrix_type& empirical_enhanced_statistic,
                                           matrix_type& default_enhanced_statistics,
                                           matrix_type& default_statistics)
      {
        assert (stats_calculator);
        default_statistics.resize (stats_calculator->num_elements(), stats_calculator->num_outputs());
        default_enhanced_statistics.resize (stats_calculator->num_elements(), stats_calculator->num_outputs());

        const matrix_type default_shuffle (matrix_type::Identity (stats_calculator->num_subjects(), stats_calculator->num_subjects()));
        (*stats_calculator) (default_shuffle, default_statistics);

        if (enhancer)
          (*enhancer) (default_statistics, default_enhanced_statistics);
        else
          default_enhanced_statistics = default_statistics;

        if (empirical_enhanced_statistic.size())
          default_enhanced_statistics.array() /= empirical_enhanced_statistic.array();
      }




      void run_permutations (const std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator,
                             const std::shared_ptr<EnhancerBase> enhancer,
                             const matrix_type& empirical_enhanced_statistic,
                             const matrix_type& default_enhanced_statistics,
                             matrix_type& perm_dist,
                             count_matrix_type& perm_dist_contributions,
                             matrix_type& uncorrected_pvalues)
      {
        assert (stats_calculator);
        Math::Stats::Shuffler shuffler (stats_calculator->num_subjects(), false, "Running permutations");
        perm_dist.resize (shuffler.size(), stats_calculator->num_outputs());
        perm_dist_contributions = count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_outputs());

        count_matrix_type global_uncorrected_pvalue_count (count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_outputs()));
        {
          Processor processor (stats_calculator, enhancer,
                               empirical_enhanced_statistic,
                               default_enhanced_statistics,
                               perm_dist,
                               perm_dist_contributions,
                               global_uncorrected_pvalue_count);
          Thread::run_queue (shuffler, Math::Stats::Shuffle(), Thread::multi (processor));
        }
        uncorrected_pvalues = global_uncorrected_pvalue_count.cast<default_type>() / default_type(shuffler.size());
      }




    }
  }
}

