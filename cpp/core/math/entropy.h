/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#pragma once

#include "math/math.h"
#include "types.h"
#include <Eigen/Dense>
#include <cmath>
#include <type_traits>

namespace MR::Math::Entropy {

enum class log_base_t { TWO, E, TEN };

namespace {
template <class Cont>
typename std::enable_if<!is_eigen_type<Cont>::value, default_type>::type prob_norm(const Cont &data) {
  default_type sum(0.0);
  for (ssize_t i = 0; i != data.size(); ++i) {
    if (std::isinf(data[i]) && data[i] > 0.0)
      throw Exception("Cannot normalise probabilities in presence of +inf values");
    if (std::isfinite(data[i])) // Ignore -inf, NaN
      sum += std::max(0.0, static_cast<default_type>(data[i]));
  }
  if (sum == 0.0)
    throw Exception("Cannot compute probability normalisation of vector with no positive values");
  return 1.0 / sum;
}

template <class Cont>
typename std::enable_if<is_eigen_type<Cont>::value, default_type>::type prob_norm(const Cont &data) {
  using Scalar = typename Cont::Scalar;
  const default_type sum = data.array()
                               .unaryExpr([](Scalar x) -> Scalar {
                                 if (std::isinf(x) && x > Scalar(0))
                                   throw Exception("Cannot normalise probabilities in presence of +inf values");
                                 return std::isfinite(x) ? std::max(Scalar(0), x) : Scalar(0); // Ignore -inf, NaN
                               })
                               .sum();
  if (sum == 0.0)
    throw Exception("Cannot compute probability normalisation of vector with no positive values");
  return 1.0 / sum;
}

} // namespace

template <class Cont, log_base_t logbase> default_type entropy(const Cont &data) {
  const default_type norm = prob_norm(data);
  default_type result(0.0);
  for (ssize_t i = 0; i != data.size(); ++i) {
    if (std::isfinite(data[i])) {
      const default_type p = std::max(0.0, norm * data[i]);
      if (p > 0.0) {
        switch (logbase) {
        case log_base_t::TWO:
          result += p * std::log2(p);
          break;
        case log_base_t::E:
          result += p * std::log(p);
          break;
        case log_base_t::TEN:
          result += p * std::log10(p);
          break;
        default:
          assert(false);
        }
      }
    }
  }
  result *= -1.0;
  return result;
}

template <class Cont> default_type shannons(const Cont &data) { return entropy<Cont, log_base_t::TWO>(data); }
template <class Cont> default_type nats(const Cont &data) { return entropy<Cont, log_base_t::E>(data); }
template <class Cont> default_type hartleys(const Cont &data) { return entropy<Cont, log_base_t::TEN>(data); }

} // namespace MR::Math::Entropy
