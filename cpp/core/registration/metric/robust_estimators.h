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

#pragma once

namespace MR {
namespace Math {
template <typename T> inline int sgn(T val) { return (T(0) < val) - (val < T(0)); }
} // namespace Math
namespace Registration {
namespace Metric {
class L1 {
public:
  void operator()(const default_type &x, default_type &residual, default_type &slope) {
    residual = abs(x);
    slope = Math::sgn(x);
  }

  void operator()(const Eigen::Matrix<default_type, Eigen::Dynamic, 1> &x,
                  Eigen::Matrix<default_type, Eigen::Dynamic, 1> &residual,
                  Eigen::Matrix<default_type, Eigen::Dynamic, 1> &slope) {
    const ssize_t len = x.size();
    residual = x.cwiseAbs();
    slope.resize(len);
    for (ssize_t i = 0; i < len; ++i) {
      slope[i] = Math::sgn(x[i]);
    }
  }
};

class L2 {
public:
  void operator()(const default_type &x, default_type &residual, default_type &slope) {
    residual = x * x;
    slope = x;
  }

  void operator()(const Eigen::Matrix<default_type, Eigen::Dynamic, 1> &x,
                  Eigen::Matrix<default_type, Eigen::Dynamic, 1> &residual,
                  Eigen::Matrix<default_type, Eigen::Dynamic, 1> &slope) {
    residual = x.cwiseAbs2(); //.squaredNorm(); //.array().square();
    slope = x;
  }
};

// least powers: residual = |x|^power with power between 1 and 2
class LP {
public:
  LP(const default_type p) : power(p) { assert(power >= 1.0 && power <= 2.0); }
  LP() : power(1.2) {}

  void operator()(const default_type &x, default_type &residual, default_type &slope) {
    residual = std::pow(abs(x), power);
    slope = Math::sgn(x) * std::pow(abs(x), power - 1.0);
  }

  void operator()(const Eigen::Matrix<default_type, Eigen::Dynamic, 1> &x,
                  Eigen::Matrix<default_type, Eigen::Dynamic, 1> &residual,
                  Eigen::Matrix<default_type, Eigen::Dynamic, 1> &slope) {
    residual = x.cwiseAbs().array().pow(power);
    slope = x.cwiseAbs().array().pow(power - 1.0);
    const ssize_t len = x.size();
    for (ssize_t i = 0; i < len; ++i) {
      slope[i] *= Math::sgn(x[i]);
    }
  }

private:
  default_type power;
};

} // namespace Metric
} // namespace Registration
} // namespace MR
