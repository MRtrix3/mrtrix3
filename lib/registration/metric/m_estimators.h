/*
   Copyright 2012 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt, 24/02/2012

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

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
      class L1 {
        public:
          void operator() (const default_type x,
                           default_type& residual,
                           default_type& slope) {
            residual = std::abs(x);
            slope = Math::sgn(x);
          }

          void operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1> x,
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

      class L2 {
        public:
          void operator() (const default_type x,
                           default_type& residual,
                           default_type& slope) {
            residual = x * x;
            slope = x;
          }

          void operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1> x,
                           Eigen::Matrix<default_type, Eigen::Dynamic, 1>& residual,
                           Eigen::Matrix<default_type, Eigen::Dynamic, 1>& slope) {
            residual = x.cwiseAbs2(); //.squaredNorm(); //.array().square();
            slope = x;
          }
      };

      // least powers: residual = |x|^power with power between 1 and 2
      class LP {
        public:
          LP (const default_type p) : power(p) {assert (power>=1.0 && power <= 2.0);}
          LP () : power(1.2) {}

          void operator() (const default_type x,
                           default_type& residual,
                           default_type& slope) {
            residual = std::pow(std::abs(x), power);
            slope = Math::sgn(x) * std::pow(std::abs(x), power - 1.0);
          }

          void operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1> x,
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
