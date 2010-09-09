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

#ifndef __dwi_tractography_iFOD_calibrator_h__
#define __dwi_tractography_iFOD_calibrator_h__

#include "point.h"
#include "math/SH.h"

#define SQRT_3_OVER_2 0.866025403784439
#define NUM_CALIBRATE 1000

namespace MR {
  namespace DWI {
    namespace Tractography {

      template <typename value_type>
        std::vector<Point<value_type> > direction_grid (value_type max_angle, value_type num)
        {
          using namespace Math;
          value_type spacing = max_angle / num;
          const value_type maxR = pow2 (num);
          std::vector<Point<value_type> > list;
          ssize_t extent = ceil (num);

          for (ssize_t i = -extent; i <= extent; ++i) {
            for (ssize_t j = -extent; j <= extent; ++j) {
              value_type x = i + 0.5*j, y = SQRT_3_OVER_2*j;
              value_type n = pow2(x) + pow2(y);
              if (n > maxR) 
                continue;
              n = spacing*Math::sqrt (n);
              value_type z = Math::cos (n);
              if (n) n = spacing * Math::sin (n) / n;
              list.push_back (Point<value_type> (n*x, n*y, z));
            }
          }

          return (list);
        }


      template <class Method> 
        void calibrate (Method& method) 
        {
          using namespace Math;

          typename Method::Calibrate calibrate_func (method);
          value_type peak = calibrate_func.get_peak();

          method.pos.set (0.0, 0.0, 0.0);
          method.dir.set (0.0, 0.0, 1.0);

          Vector<value_type> fod (method.values, method.source.dim(3));

          {
            ProgressBar progress ("calibrating rejection sampling...");

            for (value_type extent = 1.0; extent < 5.0; extent += 0.5) {
              value_type min = std::numeric_limits<value_type>::infinity();
              method.calibrate_list = direction_grid (method.S.max_angle, extent);

              for (size_t n = 0; n < NUM_CALIBRATE; ++n) {
                value_type max = 0.0;
                Point<value_type> end_dir = method.rand_dir (method.dir);

                for (size_t i = 0; i < method.calibrate_list.size(); ++i) {
                  value_type prob = calibrate_func (end_dir, method.calibrate_list[i]);

                  if (prob > max) 
                    max = prob;

                  ++progress;
                }

                if (max < min)
                  min = max;
              }

              method.calibrate_ratio = 1.1 * peak / min;

              if (method.calibrate_ratio < 3.0) 
                break;
            }
          }

          info ("rejection sampling will use " + str (method.calibrate_list.size()) 
              + " directions with a ratio of " + str (method.calibrate_ratio));
        }

    }
  }
}

#endif




