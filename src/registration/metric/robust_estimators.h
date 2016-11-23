/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */

#ifndef __registration_metric_robust_estimators_h__
#define __registration_metric_robust_estimators_h__

namespace MR
{
 namespace Math
 {
    template <typename T> inline int sgn(T val) {
      return (T(0) < val) - (val < T(0));
    }
 }
  namespace Registration
  {
    namespace Metric
    {
      class L1 { NOMEMALIGN
        public:
          void operator() (const default_type& x,
                           default_type& residual,
                           default_type& slope) {
            residual = std::abs(x);
            slope = Math::sgn(x);
          }

          void operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
                           Eigen::Matrix<default_type, Eigen::Dynamic, 1>& residual,
                           Eigen::Matrix<default_type, Eigen::Dynamic, 1>& slope) {
            const ssize_t len = x.size();
            residual = x.cwiseAbs();
            slope.resize(len);
            for (ssize_t i = 0; i < len; ++i){
              slope[i] = Math::sgn(x[i]);
            }
          }
      };

      class L2 { NOMEMALIGN
        public:
          void operator() (const default_type& x,
                           default_type& residual,
                           default_type& slope) {
            residual = x * x;
            slope = x;
          }

          void operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
                           Eigen::Matrix<default_type, Eigen::Dynamic, 1>& residual,
                           Eigen::Matrix<default_type, Eigen::Dynamic, 1>& slope) {
            residual = x.cwiseAbs2(); //.squaredNorm(); //.array().square();
            slope = x;
          }
      };

      // least powers: residual = |x|^power with power between 1 and 2
      class LP { NOMEMALIGN
        public:
          LP (const default_type p) : power(p) {assert (power>=1.0 && power <= 2.0);}
          LP () : power(1.2) {}

          void operator() (const default_type& x,
                           default_type& residual,
                           default_type& slope) {
            residual = std::pow(std::abs(x), power);
            slope = Math::sgn(x) * std::pow(std::abs(x), power - 1.0);
          }

          void operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
                           Eigen::Matrix<default_type, Eigen::Dynamic, 1>& residual,
                           Eigen::Matrix<default_type, Eigen::Dynamic, 1>& slope) {
            residual = x.cwiseAbs().array().pow(power);
            slope = x.cwiseAbs().array().pow(power -1.0);
            const ssize_t len = x.size();
            for (ssize_t i = 0; i < len; ++i){
              slope[i] *= Math::sgn(x[i]);
            }
          }

        private:
          default_type power;
      };

    }
  }
}
#endif
