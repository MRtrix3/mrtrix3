/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 25/02/09.

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

#ifndef __math_hermite_h__
#define __math_hermite_h__

#include <limits>

namespace MR {
  namespace Math {

    template <typename T> class Hermite 
    {
      public:
        typedef T value_type;

        Hermite (value_type tension = 0.0) : t (T(0.5)*tension) { }
        
        void set (value_type position) { 
          value_type p2 = position*position;
          value_type p3 = position*p2;
          w[0] = (T(0.5)-t) * (T(2.0)*p2  - p3 - position);
          w[1] = T(1.0) + (T(1.5)+t)*p3 - (T(2.5)+t)*p2;
          w[2] = (T(2.0)+T(2.0)*t)*p2 + (T(0.5)-t)*position - (T(1.5)+t)*p3;
          w[3] = (T(0.5)-t) * (p3 - p2);
        }

        value_type coef (size_t i) const { return (w[i]); }

        template <class S> 
          value_type value (const S* vals) const { return (value (vals[0], vals[1], vals[2], vals[3])); }

        template <class S> 
          value_type value (const S& a, const S& b, const S& c, const S& d) const { return (w[0]*a + w[1]*b + w[2]*c + w[3]*d); }

      private:
        value_type w[4];
        value_type t;



    };

  }
}

#endif
