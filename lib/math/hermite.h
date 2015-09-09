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

#include <vector>
#include <algorithm>

#include "math/math.h"

#include "types.h"

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
            value_type inv_cpd1 = (1.0 - ValueType(2.0)*t) * (control_points[2]-control_points[1]) / (control_points[2] - control_points[0]);
            value_type inv_cpd2 = (1.0 - ValueType(2.0)*t) * (control_points[2]-control_points[1]) / (control_points[3] - control_points[1]);
            w[0] = (ValueType(2.0)*p2  - p3 - position) * inv_cpd1;
            w[1] = ValueType(2.0)*p3 - ValueType(3.0)*p2 + ValueType(1.0) + (p2-p3)*inv_cpd2;
            w[2] = (p3 - ValueType(2.0)*p2 + position)*inv_cpd1 - ValueType(2.0)*p3 + ValueType(3.0)*p2;
            w[3] = (p3-p2)*inv_cpd2;
          }

        value_type coef (size_t i) const {
          return w[i];
        }
        value_type tension () const { 
          return 2.0*t;
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

        explicit HermiteSplines (value_type tension = 0.0) : 
          H (tension) { } 

        template <class ControlPoints>
          HermiteSplines (const ControlPoints& control_points, value_type tension = 0.0) : 
            H (tension) {
              init (control_points);
            }

        template <class ControlPoints>
          void init (const ControlPoints& control_points) {
            cp.resize (control_points.size()+2);
              if (control_points.size() < 3)
                throw Exception ("need at least 3 control points for Hermite spline curve");
              for (size_t n = 0; n < control_points.size(); ++n)
                cp[n+1] = control_points[n];
              cp[0] = 2.0*cp[1]-cp[2];
              cp[cp.size()-1] = 2.0*cp[cp.size()-2] - cp[cp.size()-3];
            }

        void clear () {
          cp.clear(); 
        }

        const std::vector<ValueType>& control_points () const {
          return cp;
        }

        void set (value_type position) {
          if (cp.size()) {
            typename std::vector<ValueType>::const_iterator it = std::find_if (cp.begin(), cp.end(), GreaterThan<ValueType> (position));
            index = it - cp.begin() - 2;
            if (index < 0) index = 0;
            if (index > ssize_t(cp.size())-4) index = cp.size()-4;
            H.set ((position-cp[index+1])/(cp[index+2]-cp[index+1]), &cp[index]);
          }
          else {
            assert (position >= 0.0);
            index = std::floor<size_t> (position);
            H.set (position - std::floor (position));
          }
        }

        template <class ArrayType>
          ValueType value (const ArrayType& intensities) const {
            value_type a = index > 0 ? intensities[index-1] : project_down (intensities[0], intensities[1], intensities[2]);
            value_type d = index < ssize_t(cp.size())-4 ? intensities[index+2] : project_up (intensities[cp.size()-5], intensities[cp.size()-4], intensities[cp.size()-3]);
            return H.value (a, intensities[index], intensities[index+1], d);
        }

      private:
        Hermite<ValueType> H;
        std::vector<ValueType> cp;
        ssize_t index;

        value_type project_down (value_type a, value_type b, value_type c) const {
          return 2.5*a - 1.5*b + 0.5*(c-b)*(cp[2]-cp[1])/(cp[3]-cp[2]); 
        }
        value_type project_up (value_type a, value_type b, value_type c) const {
          return 2.5*c - 1.5*b - 0.5*(b-a)*(cp[cp.size()-2]-cp[cp.size()-3])/(cp[cp.size()-3]-cp[cp.size()-4]); 
        }
    };

  }
}

#endif
