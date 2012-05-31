/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __quaternion_h__
#define __quaternion_h__

#include "mrtrix.h"

namespace MR
{
  namespace Math
  {

    template <typename T>
    class Quaternion
    {
      public:
        typedef T value_type;
        Quaternion () {
          reset ();
        }
        Quaternion (value_type t, value_type vx, value_type vy, value_type vz) {
          x[0] = t;
          x[1] = vx;
          x[2] = vy;
          x[3] = vz;
        }
        Quaternion (value_type b, value_type c, value_type d) {
          x[0] = sqrt (1.0 - b*b - c*c - d*d);
          x[1] = b;
          x[2] = c;
          x[3] = d;
        }
        Quaternion (value_type angle, const value_type* axis) {
          x[0] = cos (angle/2.0);
          x[1] = axis[0];
          x[2] = axis[1];
          x[3] = axis[2];
          value_type norm = sin (angle/2.0) / sqrt (x[1]*x[1] + x[2]*x[2] + x[3]*x[3]);
          x[1] *= norm;
          x[2] *= norm;
          x[3] *= norm;
        }

        Quaternion (const value_type* matrix) {
          from_matrix (matrix);
        }

        operator bool () const {
          return ! (isnan (x[0]) || isnan (x[1]) || isnan (x[2]) || isnan (x[3]));
        }
        bool operator! () const {
          return isnan (x[0]) || isnan (x[1]) || isnan (x[2]) || isnan (x[3]);
        }

        void invalidate ()  {
          x[0] = x[1] = x[2] = x[3] = NAN;
        }

        void reset () {
          x[0] = 1.0;
          x[1] = x[2] = x[3] = 0.0;
        }

        void normalise () {
          value_type n = 1.0 / sqrt (x[0]*x[0] + x[1]*x[1] + x[2]*x[2] + x[3]*x[3]);
          x[0] *= n;
          x[1] *= n;
          x[2] *= n;
          x[3] *= n;
        }
        void from_matrix (const T* matrix);
        void to_matrix (value_type* matrix);

        const value_type&  operator[] (int index) const {
          return x[index];
        }
        value_type& operator[] (int index) {
          return x[index];
        }

        bool operator== (const Quaternion& y) const {
          return memcmp (x, y.x, 4*sizeof (value_type)) == 0;
        }
        bool operator!= (const Quaternion& y) const {
          return memcmp (x, y.x, 4*sizeof (value_type));
        }

        Quaternion operator* (const Quaternion& y) const;
        const Quaternion& operator*= (const Quaternion& y) {
          *this = (*this) * y;
          return *this;
        }

        /**
         * Spherical linear interpolation between the current quarterion and another
         *
         * @param  y the other quaternion to interpolate between
         * @param  t the desired fraction of the angle to be interpolated, ranging from 0..1
         * @return The interpolated quaternion
         */
        Quaternion slerp (Quaternion& y, float t) const
        {
          Quaternion q (1, 0, 0, 0);
          double cosHalfTheta = x[0] * y[0] + x[1] * y[1] + x[2] * y[2] + x[3] * y[3];
          if (abs(cosHalfTheta) >= 1.0){
            q[0] = x[0];q[1] = x[1];q[2] = x[2];q[3] = x[3];
            return q;
          }

          if (cosHalfTheta < 0) {
            y[0] = -y[0]; y[1] = -y[1]; y[2] = -y[2]; y[3] = -y[3];
            cosHalfTheta = -cosHalfTheta;
          }

          double halfTheta = acos(cosHalfTheta);
          double sinHalfTheta = sqrt(1.0 - cosHalfTheta*cosHalfTheta);
          if (fabs(sinHalfTheta) < 0.001){
            q[0] = (x[0] * 0.5 + y[0] * 0.5);
            q[1] = (x[1] * 0.5 + y[1] * 0.5);
            q[2] = (x[2] * 0.5 + y[2] * 0.5);
            q[3] = (x[3] * 0.5 + y[3] * 0.5);
            return q;
          }
          double ratioA = sin((1 - t) * halfTheta) / sinHalfTheta;
          double ratioB = sin(t * halfTheta) / sinHalfTheta;

          q[0] = (x[0] * ratioA + y[0] * ratioB);
          q[1] = (x[1] * ratioA + y[1] * ratioB);
          q[2] = (x[2] * ratioA + y[2] * ratioB);
          q[3] = (x[3] * ratioA + y[3] * ratioB);
          return (q);
        }

      protected:
        value_type   x[4];
    };


    template <typename T>
    std::ostream& operator<< (std::ostream& stream, const Quaternion<T>& q);












    template <typename T>
    inline Quaternion<T> Quaternion<T>::operator* (const Quaternion& y) const
    {
      Quaternion q (
        x[0]*y[0] - x[1]*y[1] - x[2]*y[2] - x[3]*y[3],
        x[0]*y[1] + x[1]*y[0] + x[2]*y[3] - x[3]*y[2],
        x[0]*y[2] - x[1]*y[3] + x[2]*y[0] + x[3]*y[1],
        x[0]*y[3] + x[1]*y[2] - x[2]*y[1] + x[3]*y[0]);
      q.normalise();
      return q;
    }






    template <typename T>
    inline void Quaternion<T>::from_matrix (const T* matrix)
    {
      x[0] = 1.0 + matrix[0] + matrix[4] + matrix[8];
      x[0] = x[0] > 0.0 ? 0.5 * sqrt (x[0]) : 0.0;
      if (fabs (x[0]) < 0.1) {
        x[1] = 1.0 + matrix[0] - matrix[4] - matrix[8];
        x[1] = x[1] > 0.0 ? 0.5 * sqrt (x[1]) : 0.0;
        if (fabs (x[1]) < 0.1) {
          x[2] = 1.0 - matrix[0] + matrix[4] - matrix[8];
          x[2] = x[2] > 0.0 ? 0.5 * sqrt (x[2]) : 0.0;
          if (fabs (x[2]) < 0.1) {
            x[3] = 0.5 * sqrt (1.0 - matrix[0] - matrix[4] + matrix[8]);
            x[0] = (matrix[3] - matrix[1]) / (4.0 * x[3]);
            x[1] = (matrix[2] + matrix[6]) / (4.0 * x[3]);
            x[2] = (matrix[7] + matrix[5]) / (4.0 * x[3]);
          }
          else {
            x[0] = (matrix[2] - matrix[6]) / (4.0 * x[2]);
            x[1] = (matrix[3] + matrix[1]) / (4.0 * x[2]);
            x[3] = (matrix[7] + matrix[5]) / (4.0 * x[2]);
          }
        }
        else {
          x[0] = (matrix[7] - matrix[5]) / (4.0 * x[1]);
          x[2] = (matrix[3] + matrix[1]) / (4.0 * x[1]);
          x[3] = (matrix[2] + matrix[6]) / (4.0 * x[1]);
        }
      }
      else {
        x[1] = (matrix[7] - matrix[5]) / (4.0 * x[0]);
        x[2] = (matrix[2] - matrix[6]) / (4.0 * x[0]);
        x[3] = (matrix[3] - matrix[1]) / (4.0 * x[0]);
      }

      normalise();
    }


    template <typename T>
    inline void Quaternion<T>::to_matrix (value_type* matrix)
    {
      matrix[0] = x[0]*x[0] + x[1]*x[1] - x[2]*x[2] - x[3]*x[3];
      matrix[1] = 2.0*x[1]*x[2] - 2.0*x[0]*x[3];
      matrix[2] = 2.0*x[1]*x[3] + 2.0*x[0]*x[2];
      matrix[3] = 2.0*x[1]*x[2] + 2.0*x[0]*x[3];
      matrix[4] = x[0]*x[0] + x[2]*x[2] - x[1]*x[1] - x[3]*x[3];
      matrix[5] = 2.0*x[2]*x[3] - 2.0*x[0]*x[1];
      matrix[6] = 2.0*x[1]*x[3] - 2.0*x[0]*x[2];
      matrix[7] = 2.0*x[2]*x[3] + 2*x[0]*x[1];
      matrix[8] = x[0]*x[0] + x[3]*x[3] - x[2]*x[2] - x[1]*x[1];
    }



    template <typename T>
    inline std::ostream& operator<< (std::ostream& stream, const Quaternion<T>& q)
    {
      stream << "[ " << q[0] << " " << q[1] << "i " << q[2] << "j " << q[3] << "k ]";
      return stream;
    }


  }
}

#endif

