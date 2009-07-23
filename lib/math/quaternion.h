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


    04-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * test for rounding errors in Quaternion::from_matrix().
      In certain cases, a negative number was passed to sqrt()

*/

#ifndef __quaternion_h__
#define __quaternion_h__

#include "mrtrix.h"

namespace MR {
  namespace Math {

    class Quaternion {
      protected:
        float   x[4];

      public:
        Quaternion () { x[0] = 1.0; x[1] = x[2] = x[3] = 0.0; }
        Quaternion (float t, float vx, float vy, float vz) { x[0] = t; x[1] = vx; x[2] = vy; x[3] = vz; }
        Quaternion (float b, float c, float d) { x[0] = sqrt(1.0 - b*b - c*c - d*d); x[1] = b; x[2] = c; x[3] = d; }
        Quaternion (float angle, float* axis)
        {
          x[0] = cos (angle/2.0); x[1] = axis[0]; x[2] = axis[1]; x[3] = axis[2]; 
          float norm = sin (angle/2.0) / sqrt (x[1]*x[1] + x[2]*x[2] + x[3]*x[3]);
          x[1] *= norm; x[2] *= norm; x[3] *= norm;
        }
        Quaternion (const float* matrix)             { from_matrix (matrix); }

        operator bool () const   { return (!(isnan (x[0]) || isnan (x[1]) || isnan (x[2]) || isnan (x[3]))); }
        bool          operator! () const   { return (isnan (x[0]) || isnan (x[1]) || isnan (x[2]) || isnan (x[3])); }

        void          invalidate ()  { x[0] = x[1] = x[2] = x[3] = NAN; }
        void          normalise ()
        {
          float n = 1.0 / sqrt (x[0]*x[0] + x[1]*x[1] + x[2]*x[2] + x[3]*x[3]);
          x[0] *= n; x[1] *= n; x[2] *= n; x[3] *= n;
        }
        void          from_matrix (const float* matrix);
        void          to_matrix (float* matrix);

        const float&  operator[] (int index) const    { return (x[index]); }
        float&        operator[] (int index)          { return (x[index]); }

        bool          operator== (const Quaternion& y) const { return (memcmp (x, y.x, 4*sizeof(float)) == 0); }
        bool          operator!= (const Quaternion& y) const { return (memcmp (x, y.x, 4*sizeof(float))); }

        Quaternion    operator* (const Quaternion& y) const;
        const Quaternion& operator*= (const Quaternion& y) { *this = (*this) * y; return (*this); }

    };


    std::ostream& operator<< (std::ostream& stream, const Quaternion& q);













    inline Quaternion Quaternion::operator* (const Quaternion& y) const
    {
      Quaternion q (
          x[0]*y[0] - x[1]*y[1] - x[2]*y[2] - x[3]*y[3],
          x[0]*y[1] + x[1]*y[0] + x[2]*y[3] - x[3]*y[2],
          x[0]*y[2] - x[1]*y[3] + x[2]*y[0] + x[3]*y[1],
          x[0]*y[3] + x[1]*y[2] - x[2]*y[1] + x[3]*y[0] );
      q.normalise();
      return (q);
    }






    inline void Quaternion::from_matrix (const float* matrix)
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


    inline void Quaternion::to_matrix (float* matrix)
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



    inline std::ostream& operator<< (std::ostream& stream, const Quaternion& q)
    {
      stream << "[ " << q[0] << " " << q[1] << "i " << q[2] << "j " << q[3] << "k ]";
      return (stream);
    }


  }
}

#endif

