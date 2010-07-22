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

#include "dwi/tractography/method.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

      size_t MethodBase::rng_seed;


      Point<value_type> MethodBase::random_direction (const Point<value_type>& d, value_type max_angle, value_type sin_max_angle) 
      {
        using namespace Math;

        value_type phi = 2.0 * M_PI * rng.uniform();
        value_type theta;
        do { 
          theta = max_angle * rng.uniform();
        } while (sin_max_angle * rng.uniform() > sin (theta)); 
        Point<value_type> a (sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));

        value_type n = sqrt (pow2(d[0]) + pow2(d[1]));
        if (n == 0.0) return (d[2] < 0.0 ? -a : a);

        Point<value_type> m (d[0]/n, d[1]/n, 0.0);
        Point<value_type> mp (d[2]*m[0], d[2]*m[1], -n);

        value_type alpha = a[2];
        value_type beta = a[0]*m[0] + a[1]*m[1];

        a[0] += alpha * d[0] + beta * (mp[0] - m[0]);
        a[1] += alpha * d[1] + beta * (mp[1] - m[1]);
        a[2] += alpha * (d[2]-1.0) + beta * (mp[2] - m[2]);

        return (a);
      }

    }
  }
}





