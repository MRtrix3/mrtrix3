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
  namespace Registration
  {
    namespace Metric
    {
      class L2 {
        public:
          void operator() (const default_type x,
                           default_type& residual,
                           default_type& slope) {
            residual = x * x;
            slope = x;
          }
      };


      class LP {
        public:
          template <typename T> inline int sgn(T val) {
              return (T(0) < val) - (val < T(0));
            }

          void operator() (const default_type x,
                           default_type& residual,
                           default_type& slope,
                           const default_type power = 1.2) {
            residual = std::pow(std::abs(x), power);
            slope = sgn(x) * std::pow(std::abs(x), power - 1.0);
          }
      };
    }
  }
}
#endif
