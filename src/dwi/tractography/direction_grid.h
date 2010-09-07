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

#ifndef __dwi_tractography_iFOD_direction_grid_h__
#define __dwi_tractography_iFOD_direction_grid_h__

#include "point.h"
#include "math/SH.h"

#define SQRT_3_OVER_2 0.866025403784439

namespace MR {
  namespace DWI {
    namespace Tractography {

      template <typename value_type>
        std::vector<Point<value_type> > direction_grid (value_type max_angle, ssize_t num)
        {
          using namespace Math;
          value_type spacing = max_angle / num;
          const value_type maxR = pow2 (num);
          std::vector<Point<value_type> > list;

          for (ssize_t i = -num; i <= num; ++i) {
            for (ssize_t j = -num; j <= num; ++j) {
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


      template <typename value_type>
        inline Point<value_type> get_tangent (const Point<value_type>& position, const Point<value_type>& end_dir, value_type step_size)
        {
          using namespace Math;

          if (end_dir == Point<value_type> (0.0, 0.0, 1.0))
            return (end_dir);

          value_type theta = acos (end_dir[2]);
          value_type R = step_size / theta;
          Point<value_type> C (end_dir[0], end_dir[1], 0.0);
          C.normalise();
          C *= R;

          Point<value_type> d = position - C;
          value_type a = - d[2] / d.dot (end_dir);

          d = a * end_dir;
          d[2] += 1.0;
          d.normalise();

          return (d);
        }

    }
  }
}

#endif



