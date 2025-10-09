/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "dwi/tractography/tracking/early_exit.h"

#include <string>

#include "file/config.h"
#include "types.h"

namespace MR::DWI::Tractography::Tracking {

const default_type EarlyExit::probability_threshold =
#if EIGEN_VERSION_AT_LEAST(3, 3, 0)
    1e-3;
#else
    1e-6;
#endif
const default_type EarlyExit::zvalue_threshold = -4.753408; // For a p-value of 1e-6; should be negative
const default_type EarlyExit::cease_testing_percentage = 0.20;

bool EarlyExit::operator()(const size_t num_seeds, const size_t num_tracks) {
  if (++counter != next_test)
    return false;

  // CONF option: TckgenEarlyExit
  // CONF default: 0 (false)
  // CONF Specifies whether tckgen should be terminated prematurely
  // CONF in cases where it appears as though the target number of
  // CONF accepted streamlines is not going to be met.
  if (!File::Config::get_bool("TckgenEarlyExit", false)) {
    next_test = 0;
    return false;
  }

  if ((static_cast<default_type>(num_seeds) / static_cast<default_type>(max_num_seeds) > cease_testing_percentage) ||
      (static_cast<default_type>(num_tracks) / static_cast<default_type>(max_num_tracks) > cease_testing_percentage)) {
    DEBUG(std::string("tckgen early exit: No longer testing (tracking progressed beyond ") + //
          str<int>(std::round(100.0 * cease_testing_percentage)) + "%)");                    //
    next_test = 0;
    return false;
  }
  next_test *= 2;

#ifdef TCKGEN_EARLY_EXIT_USE_FULL_BINOMIAL
  // Bayes Theorem
  // Regularised incomplete beta function is the CDF of the binomial distribution
  // Getting the probability of generating no more than (num_tracks) tracks after (num_seeds) seeds,
  //   under the assumption that sampling probability p = (max_num_tracks/max_num_seeds)
  const Eigen::Array<default_type, 2, 1> a(
      {static_cast<default_type>(num_seeds - num_tracks), static_cast<default_type>(num_seeds - num_tracks)});
  const Eigen::Array<default_type, 2, 1> b(
      {static_cast<default_type>(num_tracks + 1), static_cast<default_type>(num_tracks + 1)});
  const Eigen::Array<default_type, 2, 1> x(
      {1.0 - (static_cast<default_type>(max_num_tracks) / static_cast<default_type>(max_num_seeds)), 1.0});
  const auto incomplete_betas = Eigen::betainc(a, b, x);
  const default_type conditional = incomplete_betas[0] / incomplete_betas[1];
  // Flat prior for both hypothesis and observation:
  //   any possible value for 0 <= num_tracks <= num_seeds equally likely
  const default_type prob_hypothesis_prior = (max_num_tracks + 1.0) / (max_num_seeds + 1.0);
  const default_type prob_observation = (num_tracks + 1.0) / (num_seeds + 1.0);
  const default_type posterior = conditional * prob_hypothesis_prior / prob_observation;
  DEBUG(std::string("tckgen early exit: Target ") +                                                                  //
        str(max_num_tracks) + "/" + str(max_num_seeds) +                                                             //
        " (" + str(static_cast<default_type>(max_num_tracks) / static_cast<default_type>(max_num_seeds), 3) + ")," + //
        " current " + str(num_tracks) + "/" + str(num_seeds) +                                                       //
        " (" + str(static_cast<default_type>(num_tracks) / static_cast<default_type>(num_seeds), 3) + ")," +         //
        " conditional probability " + str(conditional, 3) + "," +                                                    //
        " hypothesis prior probability " + str(prob_hypothesis_prior, 3) + "," +                                     //
        " observation probability " + str(prob_observation, 3) + "," +                                               //
        " posterior " + str(posterior, 3));                                                                          //
  return (posterior < probability_threshold);
#else
  // Use normal approximation to the binomial distribution
  // CDF of normal distribution isn't trivial
  //   (erf() is also in <unsupported/Eigen/SpecialFunctions>)
  //   - resort to confidence intervals
  const default_type current_ratio = static_cast<default_type>(num_tracks) / static_cast<default_type>(num_seeds);
  const default_type target_ratio =
      static_cast<default_type>(max_num_tracks) / static_cast<default_type>(max_num_seeds);
  // Use (target_ratio) rather than (current_ratio) here to better handle case where
  //   no streamlines are being accepted
  const default_type variance = target_ratio * (1.0 - target_ratio) / static_cast<default_type>(num_seeds);
  const default_type threshold = target_ratio + (TCKGEN_EARLY_EXIT_ZVALUE * std::sqrt(variance));
  DEBUG("tckgen early exit: Target " + str(max_num_tracks) + "/" + str(max_num_seeds) + " (" + str(target_ratio, 3) +
        "), " + "current " + str(num_tracks) + "/" + str(num_seeds) + " (" + str(current_ratio, 3) + "), " +
        "variance " + str(variance, 3) + ", threshold " + str(threshold, 3));
  return (current_ratio < threshold);
#endif
}

} // namespace MR::DWI::Tractography::Tracking
