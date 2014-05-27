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
#include "math/math.h"

namespace MR
{
  namespace Math
  {

    template <typename ValueType> 
      class Hermite
    {
      public:
        typedef ValueType value_type;

        Hermite (ValueType tension = 0.0) : t (ValueType (0.5) *tension) { }

        void set (value_type position) {
          value_type p2 = position*position;
          value_type p3 = position*p2;
          w[0] = (ValueType(0.5)-t) * (ValueType(2.0)*p2  - p3 - position);
          w[1] = ValueType(1.0) + (ValueType(1.5)+t)*p3 - (ValueType(2.5)+t)*p2;
          w[2] = (ValueType(2.0) + ValueType(2.0)*t)*p2 + (ValueType(0.5)-t)*position - (ValueType(1.5)+t)*p3;
          w[3] = (ValueType(0.5)-t) * (p3 - p2);
        }

        template <class ControlPoints>
          void set (value_type position, const ControlPoints& control_points) {
            value_type p2 = position*position;
            value_type p3 = position*p2;
            value_type inv_cpd1 = (1.0 - ValueType(2.0)*t) / (control_points[2] - control_points[0]);
            value_type inv_cpd2 = (1.0 - ValueType(2.0)*t) / (control_points[3] - control_points[1]);
          w[0] = (ValueType(0.5)-t) * (ValueType(2.0)*p2  - p3 - position);
          w[1] = ValueType(1.0) + (ValueType(1.5)+t)*p3 - (ValueType(2.5)+t)*p2;
          w[2] = (ValueType(2.0) + ValueType(2.0)*t)*p2 + (ValueType(0.5)-t)*position - (ValueType(1.5)+t)*p3;
          w[3] = (ValueType(0.5)-t) * (p3 - p2);
            //w[0] = (ValueType(2.0)*p2  - p3 - position) * inv_cpd1;
            //w[1] = ValueType(2.0)*p3 - ValueType(3.0)*p2 + ValueType(1.0) + (p2-p3)*inv_cpd2;
            //w[2] = (p3 - ValueType(2.0)*p2 + position)*inv_cpd1 - ValueType(2.0)*p3 + ValueType(3.0)*p2;
            //w[3] = (p3-p2)*inv_cpd2;
          }

        value_type coef (size_t i) const {
          return w[i];
        }

        template <class S>
          S value (const S* vals) const {
            return value (vals[0], vals[1], vals[2], vals[3]);
          }

        template <class S>
          S value (const S& a, const S& b, const S& c, const S& d) const {
            return w[0]*a + w[1]*b + w[2]*c + w[3]*d;
          }

      private:
        value_type w[4];
        value_type t;
    };



    namespace { 
      template <typename ValueType>
        class GreaterThan {
          public:
            GreaterThan (ValueType ref) : ref (ref) { }
            bool operator() (ValueType x) const { return x > ref; }
          private:
            ValueType ref;
        };
    }





    template <typename ValueType> 
      class HermiteSplines
    {
      public:
        typedef ValueType value_type;

          HermiteSplines (value_type tension = 0.0) : 
            H (tension) { } 

        template <class ControlPoints>
          HermiteSplines (const ControlPoints& control_points, value_type tension) : 
            H (tension), cp (control_points.size()) { 
              if (cp.size() < 4)
                throw Exception ("need at least 4 control points for Hermite spline curve");
              for (size_t n = 0; n < cp.size(); ++n)
                cp[n] = control_points[n];
            }

        void set (value_type position) {
          if (cp.size()) {
            typename std::vector<ValueType>::const_iterator it = std::find_if (cp.begin(), cp.end(), GreaterThan<ValueType> (position));
            index = it - cp.begin() - 1;
            if (index < 1) index = 1;
            if (index > cp.size()-2) index = cp.size()-2;
            index = 1;
            H.set ((position-cp[index])/(cp[index+1]-cp[index]), &cp[index-1]);
          }
          else {
            assert (position >= 0.0);
            index = Math::floor<size_t> (position);
            H.set (position - Math::floor (position));
          }
        }

        template <class ArrayType>
          ValueType value (const ArrayType& intensities) const {
            return H.value (intensities[index-1], intensities[index], intensities[index+1], intensities[index+2]);
        }

      private:
        Hermite<ValueType> H;
        std::vector<ValueType> cp;
        size_t index;
/*
        ValueType fit_quadratic (ValueType xn, ValueType x0, ValueType y0, ValueType x1, ValueType y1, ValueType x2, ValueType y2) {
          ValueType y0_y1 = y0-y1;
          ValueType x0_x1 = x0-x1;
          ValueType x02_x12 = x0*x0 - x1*x1;

          ValueType a = (y0_y1 - x0_x1*y2/x2) / ;
        }
        */
    };

  }
}

#endif
