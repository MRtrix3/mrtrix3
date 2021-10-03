/* Copyright (c) 2008-2021 the MRtrix3 contributors.
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

#include "stats/permtest.h"

namespace MR
{
  namespace Stats
  {
    namespace PermTest
    {



      const char* const mask_posthoc_description = 
          "When performing statistical inference, there is a subtle difference between operation of the -mask and "
          "-posthoc options. Elements excluded from the former will be excluded from intermediate outputs, will "
          "not contribute to statistical enhancement, and will therefore not contribute to construction of the null "
          "distribution(s) and will not be assigned corrected p-values. On the other hand, elements excluded "
          "from the latter will still contribute to statistical enhancement, and will instead only be excluded from "
          "construction of the null distribution(s) and calculation of p-values. The latter is intended to be used "
          "in post hoc statistical tests, where a prior inference step has identified a subset of statistically "
          "significant elements and one seeks to isolate the contributing factors; within such a step, elements for "
          "which statistical significance was not assigned should nevertheless still contribute to statistical "
          "enhancement as they did in the prior inference.";




      PreProcessor::PreProcessor (const std::unique_ptr<Math::Stats::GLM::TestBase>& stats_calculator,
                                  const std::shared_ptr<EnhancerBase> enhancer,
                                  const default_type skew,
                                  matrix_type& global_enhanced_sum,
                                  count_matrix_type& global_enhanced_count) :
          stats_calculator (stats_calculator->__clone()),
          enhancer (enhancer),
          skew (skew),
          global_enhanced_sum (global_enhanced_sum),
          global_enhanced_count (global_enhanced_count),
          enhanced_sum (matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_hypotheses())),
          enhanced_count (count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_hypotheses())),
          stats (global_enhanced_sum.rows(), global_enhanced_sum.cols()),
          zstats (global_enhanced_sum.rows(), global_enhanced_sum.cols()),
          enhanced_stats (global_enhanced_sum.rows(), global_enhanced_sum.cols()),
          mutex (new std::mutex())
      {
        assert (stats_calculator);
        assert (enhancer);
      }



      PreProcessor::PreProcessor (const PreProcessor& that) :
          stats_calculator (that.stats_calculator->__clone()),
          enhancer (that.enhancer),
          skew (that.skew),
          global_enhanced_sum (that.global_enhanced_sum),
          global_enhanced_count (that.global_enhanced_count),
          enhanced_sum (matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_hypotheses())),
          enhanced_count (count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_hypotheses())),
          stats (global_enhanced_sum.rows(), global_enhanced_sum.cols()),
          zstats (global_enhanced_sum.rows(), global_enhanced_sum.cols()),
          enhanced_stats (global_enhanced_sum.rows(), global_enhanced_sum.cols()),
          mutex (that.mutex) { }



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
        (*stats_calculator) (shuffle.data, stats, zstats);
        (*enhancer) (zstats, enhanced_stats);
        for (size_t ih = 0; ih != stats_calculator->num_hypotheses(); ++ih) {
          for (size_t ie = 0; ie != stats_calculator->num_elements(); ++ie) {
            if (enhanced_stats(ie, ih) > 0.0) {
              enhanced_sum(ie, ih) += std::pow (enhanced_stats(ie, ih), skew);
              enhanced_count(ie, ih)++;
            }
          }
        }
        return true;
      }







      Processor::Processor (const std::unique_ptr<Math::Stats::GLM::TestBase>& stats_calculator,
                            const std::shared_ptr<EnhancerBase> enhancer,
                            const matrix_type& empirical_enhanced_statistics,
                            const matrix_type& default_enhanced_statistics,
                            const mask_type& mask,
                            matrix_type& perm_dist,
                            count_matrix_type& perm_dist_contributions,
                            count_matrix_type& global_uncorrected_pvalue_counter) :
          stats_calculator (stats_calculator->__clone()),
          enhancer (enhancer),
          empirical_enhanced_statistics (empirical_enhanced_statistics),
          default_enhanced_statistics (default_enhanced_statistics),
          mask (mask),
          statistics (stats_calculator->num_elements(), stats_calculator->num_hypotheses()),
          zstatistics (stats_calculator->num_elements(), stats_calculator->num_hypotheses()),
          enhanced_statistics (stats_calculator->num_elements(), stats_calculator->num_hypotheses()),
          null_dist (perm_dist),
          global_null_dist_contributions (perm_dist_contributions),
          null_dist_contribution_counter (count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_hypotheses())),
          global_uncorrected_pvalue_counter (global_uncorrected_pvalue_counter),
          uncorrected_pvalue_counter (count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_hypotheses())),
          mutex (new std::mutex())
      {
        assert (stats_calculator);
      }



      Processor::Processor (const Processor& that) :
          stats_calculator (that.stats_calculator->__clone()),
          enhancer (that.enhancer),
          empirical_enhanced_statistics (that.empirical_enhanced_statistics),
          default_enhanced_statistics (that.default_enhanced_statistics),
          mask (that.mask),
          statistics (stats_calculator->num_elements(), stats_calculator->num_hypotheses()),
          zstatistics (stats_calculator->num_elements(), stats_calculator->num_hypotheses()),
          enhanced_statistics (stats_calculator->num_elements(), stats_calculator->num_hypotheses()),
          null_dist (that.null_dist),
          global_null_dist_contributions (that.global_null_dist_contributions),
          null_dist_contribution_counter (count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_hypotheses())),
          global_uncorrected_pvalue_counter (that.global_uncorrected_pvalue_counter),
          uncorrected_pvalue_counter (count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_hypotheses())),
          mutex (that.mutex) { }





      Processor::~Processor ()
      {
        std::lock_guard<std::mutex> lock (*mutex);
        global_uncorrected_pvalue_counter += uncorrected_pvalue_counter;
        global_null_dist_contributions += null_dist_contribution_counter;
      }



      bool Processor::operator() (const Math::Stats::Shuffle& shuffle)
      {
        (*stats_calculator) (shuffle.data, statistics, zstatistics);
        if (enhancer)
          (*enhancer) (zstatistics, enhanced_statistics);
        else
          enhanced_statistics = zstatistics;

        if (empirical_enhanced_statistics.size())
          enhanced_statistics.array() /= empirical_enhanced_statistics.array();

        if (null_dist.cols() == 1) { // strong fwe control
          ssize_t max_element, max_hypothesis;
          null_dist(shuffle.index, 0) = (enhanced_statistics.array().colwise() * mask.cast<matrix_type::Scalar>()).maxCoeff (&max_element, &max_hypothesis);
          null_dist_contribution_counter(max_element, max_hypothesis)++;
        } else { // weak fwe control
          ssize_t max_index;
          for (ssize_t ih = 0; ih != enhanced_statistics.cols(); ++ih) {
            null_dist(shuffle.index, ih) = (enhanced_statistics.col (ih).array() * mask.cast<matrix_type::Scalar>()).maxCoeff (&max_index);
            null_dist_contribution_counter(max_index, ih)++;
          }
        }

        for (ssize_t ih = 0; ih != enhanced_statistics.cols(); ++ih) {
          for (ssize_t ie = 0; ie != enhanced_statistics.rows(); ++ie) {
            if (mask[ie] && default_enhanced_statistics(ie, ih) > enhanced_statistics(ie, ih))
              uncorrected_pvalue_counter(ie, ih)++;
          }
        }

        return true;
      }







      void precompute_empirical_stat (const std::unique_ptr<Math::Stats::GLM::TestBase>& stats_calculator,
                                      const std::shared_ptr<EnhancerBase> enhancer,
                                      const default_type skew,
                                      matrix_type& empirical_statistic)
      {
        assert (stats_calculator);
        empirical_statistic = matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_hypotheses());
        count_matrix_type global_enhanced_count (count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_hypotheses()));
        {
          Math::Stats::Shuffler shuffler (stats_calculator->num_inputs(), true, "Pre-computing empirical statistic for non-stationarity correction");
          PreProcessor preprocessor (stats_calculator, enhancer, skew, empirical_statistic, global_enhanced_count);
          Thread::run_queue (shuffler, Math::Stats::Shuffle(), Thread::multi (preprocessor));
        }
        for (size_t contrast = 0; contrast != stats_calculator->num_hypotheses(); ++contrast) {
          for (size_t ie = 0; ie != stats_calculator->num_elements(); ++ie) {
            if (global_enhanced_count(ie, contrast) > 0)
              empirical_statistic(ie, contrast) = std::pow (empirical_statistic(ie, contrast) / static_cast<default_type> (global_enhanced_count(ie, contrast)), 1.0/skew);
            else
              empirical_statistic(ie, contrast) = std::numeric_limits<default_type>::infinity();
          }
        }
      }




      void precompute_default_permutation (const std::unique_ptr<Math::Stats::GLM::TestBase>& stats_calculator,
                                           const std::shared_ptr<EnhancerBase> enhancer,
                                           const matrix_type& empirical_enhanced,
                                           matrix_type& output_statistics,
                                           matrix_type& output_zstats,
                                           matrix_type& output_enhanced)
      {
        assert (stats_calculator);
        ProgressBar progress (std::string("Running GLM ") + (enhancer ? "and enhancement algorithm " : "") + "for default permutation", 4);
        output_statistics.resize (stats_calculator->num_elements(), stats_calculator->num_hypotheses());
        output_zstats    .resize (stats_calculator->num_elements(), stats_calculator->num_hypotheses());
        output_enhanced  .resize (stats_calculator->num_elements(), stats_calculator->num_hypotheses());
        const matrix_type default_shuffle (matrix_type::Identity (stats_calculator->num_inputs(), stats_calculator->num_inputs()));
        ++progress;

        (*stats_calculator) (default_shuffle, output_statistics, output_zstats);
        ++progress;

        if (enhancer)
          (*enhancer) (output_zstats, output_enhanced);
        else
          output_enhanced = output_statistics;
        ++progress;

        if (empirical_enhanced.size())
          output_enhanced.array() /= empirical_enhanced.array();
      }




      void run_permutations (const std::unique_ptr<Math::Stats::GLM::TestBase>& stats_calculator,
                             const std::shared_ptr<EnhancerBase> enhancer,
                             const matrix_type& empirical_enhanced_statistic,
                             const matrix_type& default_enhanced_statistics,
                             const bool fwe_strong,
                             const mask_type& mask,
                             matrix_type& null_dist,
                             count_matrix_type& null_dist_contributions,
                             matrix_type& uncorrected_pvalues)
      {
        assert (stats_calculator);
        assert (stats_calculator->num_elements() == size_t(mask.size()));
        Math::Stats::Shuffler shuffler (stats_calculator->num_inputs(), false, "Running permutations");
        null_dist.resize (shuffler.size(), fwe_strong ? 1 : stats_calculator->num_hypotheses());
        null_dist_contributions = count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_hypotheses());

        count_matrix_type global_uncorrected_pvalue_count (count_matrix_type::Zero (stats_calculator->num_elements(), stats_calculator->num_hypotheses()));
        {
          Processor processor (stats_calculator, enhancer,
                               empirical_enhanced_statistic,
                               default_enhanced_statistics,
                               mask,
                               null_dist,
                               null_dist_contributions,
                               global_uncorrected_pvalue_count);
          Thread::run_queue (shuffler, Math::Stats::Shuffle(), Thread::multi (processor));
        }
        uncorrected_pvalues = global_uncorrected_pvalue_count.cast<default_type>() / default_type(shuffler.size());
      }




    }
  }
}

