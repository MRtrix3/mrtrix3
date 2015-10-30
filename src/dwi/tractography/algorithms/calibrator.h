/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 25/10/09.

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

#ifndef __dwi_tractography_algorithms_iFOD_calibrator_h__
#define __dwi_tractography_algorithms_iFOD_calibrator_h__

#include "math/SH.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/types.h"

#define SQRT_3_OVER_2 0.866025403784439
#define NUM_CALIBRATE 1000

namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Algorithms {

      using namespace MR::DWI::Tractography::Tracking;

      std::vector<Eigen::Vector3f> direction_grid (float max_angle, float spacing)
      {
        const float maxR = Math::pow2 (max_angle / spacing);
        std::vector<Eigen::Vector3f> list;
        ssize_t extent = std::ceil (max_angle / spacing);

        for (ssize_t i = -extent; i <= extent; ++i) {
          for (ssize_t j = -extent; j <= extent; ++j) {
            float x = i + 0.5*j, y = SQRT_3_OVER_2*j;
            float n = Math::pow2(x) + Math::pow2(y);
            if (n > maxR) 
              continue;
            n = spacing * std::sqrt (n);
            float z = std::cos (n);
            if (n) n = spacing * std::sin (n) / n;
            list.push_back ({ n*x, n*y, z });
          }
        }

        return (list);
      }

      namespace {
        class Pair {
          public:
            Pair (float elevation, float amplitude) : el (elevation), amp (amplitude) { }
            float el, amp;
        };
      }

      template <class Method> 
        void calibrate (Method& method) 
        {
          typename Method::Calibrate calibrate_func (method);
          const float sqrt3 = std::sqrt (3.0);

          std::vector<Pair> amps;
          for (float el = 0.0; el < method.S.max_angle; el += 0.001) {
            amps.push_back (Pair (el, calibrate_func (el)));
            if (!std::isfinite (amps.back().amp) || amps.back().amp <= 0.0) break;
          }
          float zero = amps.back().el;

          float N_min = Inf, theta_min = NaN, ratio = NaN;
          for (size_t i = 1; i < amps.size(); ++i) {
            float N = Math::pow2 (method.S.max_angle);
            float Ns =  N * (1.0+amps[0].amp/amps[i].amp)/(2.0*Math::pow2(zero));
            auto dirs = direction_grid (method.S.max_angle + amps[i].el, sqrt3*amps[i].el);
            N = Ns+dirs.size();
            //std::cout << amps[i].el << " " << amps[i].amp << " " << Ns << " " << dirs.size() << " " << Ns+dirs.size() << "\n";
            if (N > 0.0 && N < N_min) {
              N_min = N;
              theta_min = amps[i].el;
              ratio = amps[0].amp / amps[i].amp;
            }
          }

          method.calibrate_list = direction_grid (method.S.max_angle+theta_min, sqrt3*theta_min);
          method.calibrate_ratio = ratio;

          INFO ("rejection sampling will use " + str (method.calibrate_list.size()) 
              + " directions with a ratio of " + str (method.calibrate_ratio) + " (predicted number of samples per step = " + str (N_min) + ")");
        }


      }
    }
  }
}

#endif




