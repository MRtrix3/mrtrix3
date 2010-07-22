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

#ifndef __dwi_tractography_iFOD_calibrate_h__
#define __dwi_tractography_iFOD_calibrate_h__

#include "point.h"
#include "math/SH.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

      class Calibrator
      {
        public:
          Calibrator (value_type max_angle, value_type spacing) :
            max_a (max_angle), 
            spac (spacing) { 
              VAR (max_angle);
              VAR (spacing);
              ssize_t N = Math::ceil<ssize_t> (max_a/spac);
              const value_type maxR = Math::pow2(max_a/spac);
              for (ssize_t i = -N; i <= N; ++i) {
                for (ssize_t j = -N; j <= N; ++j) {
                  value_type x = i + 0.5*j, y = 0.5*Math::sqrt(3.0)*j;
                  value_type n = Math::pow2(x) + Math::pow2(y);
                  if (n > maxR) 
                    continue;

                  n = spac*Math::sqrt (n);
                  value_type z = Math::cos (n);
                  if (n) n = spac * Math::sin (n) / n;
                  list.push_back (Point<value_type> (n*x, n*y, z));
                }
              }

              for (size_t i = 0; i < list.size(); ++i)
                std::cout << list[i] << "\n";
            }

          size_t count () const { return (list.size()); }
          const Point<value_type>& dir (size_t index) const { return (list[index]); }

        private:
          value_type max_a, spac;
          std::vector<Point<value_type> > list;
      };

    }
  }
}

#endif


