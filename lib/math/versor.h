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

#ifndef __versor_h__
#define __versor_h__

#include "math/matrix.h"

namespace MR
{
  namespace Math
  {

    template <typename T>
    class Versor
    {
      public:
        typedef T value_type;
        Versor () {
          reset ();
        }

        Versor (value_type t, value_type vx, value_type vy, value_type vz) {
          x[0] = t;
          x[1] = vx;
          x[2] = vy;
          x[3] = vz;
          normalise();
        }

        Versor (value_type b, value_type c, value_type d) {
          x[0] = sqrt (1.0 - b*b - c*c - d*d);
          x[1] = b;
          x[2] = c;
          x[3] = d;
        }

        Versor (value_type angle, const value_type* axis) {
          x[0] = cos (angle/2.0);
          x[1] = axis[0];
          x[2] = axis[1];
          x[3] = axis[2];
          value_type norm = sin (angle/2.0) / sqrt (x[1]*x[1] + x[2]*x[2] + x[3]*x[3]);
          x[1] *= norm;
          x[2] *= norm;
          x[3] *= norm;
        }

        Versor (const Matrix<value_type>& matrix) {
          from_matrix (matrix);
        }

        Versor (const Vector<value_type>& param) {
          if (param.size() == 3) {
            x[0] = sqrt (1.0 - Math::norm2 (param));
            x[1] = param[0];
            x[2] = param[1];
            x[3] = param[2];
          }
          else if (param.size() == 4) {
            x[0] = param[0];
            x[1] = param[1];
            x[2] = param[2];
            x[3] = param[3];
            normalise();
          }
          else
            throw Exception ("unexpected Vector size in Versor initialiser!");
        }

        operator bool () const {
          return ! (std::isnan (x[0]) || std::isnan (x[1]) || std::isnan (x[2]) || std::isnan (x[3]));
        }
        bool operator! () const {
          return std::isnan (x[0]) || std::isnan (x[1]) || std::isnan (x[2]) || std::isnan (x[3]);
        }

        void invalidate ()  {
          x[0] = x[1] = x[2] = x[3] = NAN;
        }

        void reset () {
          x[0] = 1.0;
          x[1] = x[2] = x[3] = 0.0;
        }

        void normalise () {
          Math::normalise (x, 4);
        }

        void from_matrix (const Math::Matrix<value_type>& matrix);

        void to_matrix (Math::Matrix<value_type>& matrix) const;

        const value_type&  operator[] (int index) const {
          return x[index];
        }
        value_type& operator[] (int index) {
          return x[index];
        }

        bool operator== (const Versor& y) const {
          return memcmp (x, y.x, 4*sizeof (value_type)) == 0;
        }
        bool operator!= (const Versor& y) const {
          return memcmp (x, y.x, 4*sizeof (value_type));
        }

        Versor operator* (const Versor& y) const;
        const Versor& operator*= (const Versor& y) {
          *this = (*this) * y;
          return *this;
        }

        /**
         * Spherical linear interpolation between the current versor and another
         *
         * @param  i the interpolated versor
         * @param  y the other versor to interpolate between
         * @param  t the desired fraction of the angle to be interpolated, ranging from 0..1
         */
        void slerp (Versor& i, float t, const Versor& y) const;


        void set (const Vector<value_type>& axis) {
          const value_type sinangle2 =  Math::norm (axis);
          if (sinangle2 > 1.0)
            throw Exception ("trying to set a versor with magnitude greater than 1.");
          x[0] = Math::sqrt(1.0 - sinangle2 * sinangle2);
          x[1] = axis[0];
          x[2] = axis[1];
          x[3] = axis[2];
        }

        void set (const Vector<value_type>& axis, value_type angle) {
          const value_type vector_norm = Math::norm (axis);
          const value_type cosangle2 = Math::cos (angle / 2.0);
          const value_type sinangle2 = Math::sin (angle / 2.0);
          const value_type factor = sinangle2 / vector_norm;
          x[0] = cosangle2;
          x[1] = axis[0] * factor;
          x[2] = axis[1] * factor;
          x[3] = axis[2] * factor;
        }


      protected:
        value_type   x[4];
    };






    template <typename value_type>
    std::ostream& operator<< (std::ostream& stream, const Versor<value_type>& q);


    template <typename value_type>
    inline Versor<value_type> Versor<value_type>::operator* (const Versor& y) const
    {
      Versor q (
        x[0]*y[0] - x[1]*y[1] - x[2]*y[2] - x[3]*y[3],
        x[0]*y[1] + x[1]*y[0] + x[2]*y[3] - x[3]*y[2],
        x[0]*y[2] - x[1]*y[3] + x[2]*y[0] + x[3]*y[1],
        x[0]*y[3] + x[1]*y[2] - x[2]*y[1] + x[3]*y[0]);
      q.normalise();
      return q;
    }


    template <typename value_type>
    inline void Versor<value_type>::from_matrix (const Math::Matrix<value_type>& matrix)
    {
      x[0] = 1.0 + matrix(0,0) + matrix(1,1) + matrix(2,2);
      x[0] = x[0] > 0.0 ? 0.5 * sqrt (x[0]) : 0.0;
      if (fabs (x[0]) < 0.1) {
        x[1] = 1.0 + matrix(0,0) - matrix(1,1) - matrix(2,2);
        x[1] = x[1] > 0.0 ? 0.5 * sqrt (x[1]) : 0.0;
        if (fabs (x[1]) < 0.1) {
          x[2] = 1.0 - matrix(0,0) + matrix(1,1) - matrix(2,2);
          x[2] = x[2] > 0.0 ? 0.5 * sqrt (x[2]) : 0.0;
          if (fabs (x[2]) < 0.1) {
            x[3] = 0.5 * sqrt (1.0 - matrix(0,0) - matrix(1,1) + matrix(2,2));
            x[0] = (matrix(1,0) - matrix(0,1)) / (4.0 * x[3]);
            x[1] = (matrix(0,2) + matrix(2,0)) / (4.0 * x[3]);
            x[2] = (matrix(2,1) + matrix(1,2)) / (4.0 * x[3]);
          }
          else {
            x[0] = (matrix(0,2) - matrix(2,0)) / (4.0 * x[2]);
            x[1] = (matrix(1,0) + matrix(0,1)) / (4.0 * x[2]);
            x[3] = (matrix(2,1) + matrix(1,2)) / (4.0 * x[2]);
          }
        }
        else {
          x[0] = (matrix(2,1) - matrix(1,2)) / (4.0 * x[1]);
          x[2] = (matrix(1,0) + matrix(0,1)) / (4.0 * x[1]);
          x[3] = (matrix(0,2) + matrix(2,0)) / (4.0 * x[1]);
        }
      }
      else {
        x[1] = (matrix(2,1) - matrix(1,2)) / (4.0 * x[0]);
        x[2] = (matrix(0,2) - matrix(2,0)) / (4.0 * x[0]);
        x[3] = (matrix(1,0) - matrix(0,1)) / (4.0 * x[0]);
      }

      normalise();
    }


    template <typename value_type>
    inline void Versor<value_type>::to_matrix (Math::Matrix<value_type>& matrix) const
    {
      if (matrix.columns() < 3 || matrix.rows() < 3)
        matrix.allocate(3,3);
      matrix(0,0) = x[0]*x[0] + x[1]*x[1] - x[2]*x[2] - x[3]*x[3];
      matrix(0,1) = 2.0*x[1]*x[2] - 2.0*x[0]*x[3];
      matrix(0,2) = 2.0*x[1]*x[3] + 2.0*x[0]*x[2];
      matrix(1,0) = 2.0*x[1]*x[2] + 2.0*x[0]*x[3];
      matrix(1,1) = x[0]*x[0] + x[2]*x[2] - x[1]*x[1] - x[3]*x[3];
      matrix(1,2) = 2.0*x[2]*x[3] - 2.0*x[0]*x[1];
      matrix(2,0) = 2.0*x[1]*x[3] - 2.0*x[0]*x[2];
      matrix(2,1) = 2.0*x[2]*x[3] + 2*x[0]*x[1];
      matrix(2,2) = x[0]*x[0] + x[3]*x[3] - x[2]*x[2] - x[1]*x[1];
    }


    template <typename value_type>
    inline void Versor<value_type>::slerp (Versor& i, float t, const Versor& y) const
    {
      double cosHalfTheta = x[0] * y[0] + x[1] * y[1] + x[2] * y[2] + x[3] * y[3];
      if (abs(cosHalfTheta) >= 1.0) {
        i[0] = x[0]; i[1] = x[1]; i[2] = x[2]; i[3] = x[3];
        return;
      }

      if (cosHalfTheta < 0) {
        y[0] = -y[0]; y[1] = -y[1]; y[2] = -y[2]; y[3] = -y[3];
        cosHalfTheta = -cosHalfTheta;
      }

      double halfTheta = acos(cosHalfTheta);
      double sinHalfTheta = sqrt(1.0 - cosHalfTheta*cosHalfTheta);
      if (fabs(sinHalfTheta) < 0.001) {
        i[0] = (x[0] * 0.5 + y[0] * 0.5);
        i[1] = (x[1] * 0.5 + y[1] * 0.5);
        i[2] = (x[2] * 0.5 + y[2] * 0.5);
        i[3] = (x[3] * 0.5 + y[3] * 0.5);
        return;
      }
      double ratioA = sin((1 - t) * halfTheta) / sinHalfTheta;
      double ratioB = sin(t * halfTheta) / sinHalfTheta;

      i[0] = (x[0] * ratioA + y[0] * ratioB);
      i[1] = (x[1] * ratioA + y[1] * ratioB);
      i[2] = (x[2] * ratioA + y[2] * ratioB);
      i[3] = (x[3] * ratioA + y[3] * ratioB);
      return;
    }


    template <typename value_type>
    inline std::ostream& operator<< (std::ostream& stream, const Versor<value_type>& q)
    {
      stream << "[ " << q[0] << " " << q[1] << "i " << q[2] << "j " << q[3] << "k ]";
      return stream;
    }


  }
}

#endif

