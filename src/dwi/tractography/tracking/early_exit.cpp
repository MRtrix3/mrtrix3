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


#include "dwi/tractography/tracking/early_exit.h"

#include "file/config.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {



        bool EarlyExit::operator() (const size_t num_seeds, const size_t num_tracks)
        {
          if (++counter != next_test)
            return false;

          //CONF option: TckgenEarlyExit
          //CONF default: 0 (false)
          //CONF Specifies whether tckgen should be terminated prematurely
          //CONF in cases where it appears as though the target number of
          //CONF accepted streamlines is not going to be met.
          if (!File::Config::get_bool ("TckgenEarlyExit", false)) {
            next_test = 0;
            return false;
          }

          if ((num_seeds  / default_type(max_num_seeds)  > TCKGEN_EARLY_EXIT_STOP_TESTING_PERCENTAGE) ||
              (num_tracks / default_type(max_num_tracks) > TCKGEN_EARLY_EXIT_STOP_TESTING_PERCENTAGE)) {
            DEBUG ("tckgen early exit: No longer testing (tracking progressed beyond " + str<int>(std::round(100.0*TCKGEN_EARLY_EXIT_STOP_TESTING_PERCENTAGE)) + "%)");
            next_test = 0;
            return false;
          }
          next_test *= 2;

#ifdef TCKGEN_EARLY_EXIT_USE_FULL_BINOMIAL
          // Bayes Theorem
          // Regularised incomplete beta function is the CDF of the binomial distribution
          // Getting the probability of generating no more than (num_tracks) tracks after (num_seeds) seeds,
          //   under the assumption that sampling probability p = (max_num_tracks/max_num_seeds)
          const Eigen::Array<default_type, 2, 1> a ( { default_type(num_seeds-num_tracks), default_type(num_seeds-num_tracks) });
          const Eigen::Array<default_type, 2, 1> b ( { default_type(num_tracks+1), default_type(num_tracks+1) });
          const Eigen::Array<default_type, 2, 1> x ( { 1.0-(max_num_tracks/default_type(max_num_seeds)), 1.0 });
          const auto incomplete_betas = Eigen::betainc (a, b, x);
          const default_type conditional = incomplete_betas[0] / incomplete_betas[1];
          // Flat prior for both hypothesis and observation:
          //   any possible value for 0 <= num_tracks <= num_seeds equally likely
          const default_type prob_hypothesis_prior = (max_num_tracks+1.0)/(max_num_seeds+1.0);
          const default_type prob_observation = (num_tracks+1.0)/(num_seeds+1.0);
          const default_type posterior = conditional * prob_hypothesis_prior / prob_observation;
          DEBUG ("tckgen early exit: Target " + str(max_num_tracks) + "/" + str(max_num_seeds) + " (" + str(max_num_tracks/default_type(max_num_seeds), 3) + "), "
                 + "current " + str(num_tracks) + "/" + str(num_seeds) + " (" + str(num_tracks/default_type(num_seeds), 3) + "), "
                 + "conditional probability " + str(conditional, 3) + ", hypothesis prior probability " + str(prob_hypothesis_prior, 3) + ", "
                 + "observation probability " + str(prob_observation, 3) + ", posterior " + str(posterior, 3));
          return (posterior < TCKGEN_EARLY_EXIT_PROB_THRESHOLD);
#else
          // Use normal approximation to the binomial distribution
          // CDF of normal distribution isn't trivial
          //   (erf() is also in <unsupported/Eigen/SpecialFunctions>)
          //   - resort to confidence intervals
          const default_type current_ratio = default_type(num_tracks) / default_type(num_seeds);
          const default_type target_ratio = default_type(max_num_tracks) / default_type(max_num_seeds);
          // Use (target_ratio) rather than (current_ratio) here to better handle case where
          //   no streamlines are being accepted
          const default_type variance = target_ratio * (1.0-target_ratio) / default_type(num_seeds);
          const default_type threshold = target_ratio + (TCKGEN_EARLY_EXIT_ZVALUE * std::sqrt(variance));
          DEBUG ("tckgen early exit: Target " + str(max_num_tracks) + "/" + str(max_num_seeds) + " (" + str(target_ratio, 3) + "), "
                 + "current " + str(num_tracks) + "/" + str(num_seeds) + " (" + str(current_ratio, 3) + "), "
                 + "variance " + str(variance, 3) + ", threshold " + str(threshold, 3));
          return (current_ratio < threshold);
#endif
        }



      }
    }
  }
}

