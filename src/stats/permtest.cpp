/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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



      PreProcessor::PreProcessor (const std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator,
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



      bool PreProcessor::operator() (const Math::Stats::Shuffle& shuffle)
      {
        if (!shuffle.data.rows())
          return false;
        (*stats_calculator) (shuffle.data, stats);
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







      Processor::Processor (const std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator,
                            const std::shared_ptr<EnhancerBase> enhancer,
                            const matrix_type& empirical_enhanced_statistics,
                            const matrix_type& default_enhanced_statistics,
                            matrix_type& perm_dist,
                            vector<vector<size_t>>& global_uncorrected_pvalue_counter) :
          stats_calculator (stats_calculator),
          enhancer (enhancer),
          empirical_enhanced_statistics (empirical_enhanced_statistics),
          default_enhanced_statistics (default_enhanced_statistics),
          statistics (stats_calculator->num_elements(), stats_calculator->num_outputs()),
          enhanced_statistics (stats_calculator->num_elements(), stats_calculator->num_outputs()),
    // TODO Consider changing to Eigen::Array<size_t>
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
        for (size_t contrast = 0; contrast != stats_calculator->num_outputs(); ++contrast) {
          for (size_t element = 0; element != stats_calculator->num_elements(); ++element)
            global_uncorrected_pvalue_counter[contrast][element] += uncorrected_pvalue_counter[contrast][element];
        }
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

        perm_dist.row(shuffle.index) = enhanced_statistics.colwise().maxCoeff();

        for (ssize_t contrast = 0; contrast != enhanced_statistics.cols(); ++contrast) {
          for (ssize_t element = 0; element != enhanced_statistics.rows(); ++element) {
            if (default_enhanced_statistics(element, contrast) > enhanced_statistics(element, contrast))
              uncorrected_pvalue_counter[contrast][element]++;
          }
        }

        return true;
      }







      void precompute_empirical_stat (const std::shared_ptr<Math::Stats::GLM::TestBase> stats_calculator,
                                      const std::shared_ptr<EnhancerBase> enhancer,
                                      matrix_type& empirical_statistic)
      {
        assert (stats_calculator);
        empirical_statistic = matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_outputs());
        vector<vector<size_t>> global_enhanced_count (stats_calculator->num_outputs(), vector<size_t> (stats_calculator->num_elements(), 0));
        {
          Math::Stats::Shuffler shuffler (stats_calculator->num_subjects(), true, "Pre-computing empirical statistic for non-stationarity correction");
          PreProcessor preprocessor (stats_calculator, enhancer, empirical_statistic, global_enhanced_count);
          Thread::run_queue (shuffler, Math::Stats::Shuffle(), Thread::multi (preprocessor));
        }
        for (ssize_t contrast = 0; contrast != stats_calculator->num_outputs(); ++contrast) {
          for (ssize_t element = 0; element != stats_calculator->num_elements(); ++element) {
            if (global_enhanced_count[contrast][element] > 0)
              empirical_statistic(contrast, element) /= static_cast<default_type> (global_enhanced_count[contrast][element]);
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
                             matrix_type& uncorrected_pvalues)
      {
        assert (stats_calculator);
        Math::Stats::Shuffler shuffler (stats_calculator->num_subjects(), false, "Running permutations");
        perm_dist.resize (shuffler.size(), stats_calculator->num_outputs());
        uncorrected_pvalues.resize (stats_calculator->num_elements(), stats_calculator->num_outputs());

        vector<vector<size_t>> global_uncorrected_pvalue_count (stats_calculator->num_outputs(), vector<size_t> (stats_calculator->num_elements(), 0));
        {
          Processor processor (stats_calculator, enhancer,
                               empirical_enhanced_statistic,
                               default_enhanced_statistics,
                               perm_dist,
                               global_uncorrected_pvalue_count);
          Thread::run_queue (shuffler, Math::Stats::Shuffle(), Thread::multi (processor));
        }

        for (size_t contrast = 0; contrast != stats_calculator->num_outputs(); ++contrast) {
          for (size_t element = 0; element != stats_calculator->num_elements(); ++element)
            uncorrected_pvalues(element, contrast) = global_uncorrected_pvalue_count[contrast][element] / default_type(shuffler.size());
        }
      }




    }
  }
}

