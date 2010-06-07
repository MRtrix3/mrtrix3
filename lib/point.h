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

#ifndef __point_h__
#define __point_h__

#include "mrtrix.h"
#include "math/vector.h"

namespace MR {

  class Point {
    public:
      Point ()                               { set (NAN, NAN, NAN); }
      Point (float x, float y, float z)      { set (x, y, z); }
      explicit Point (const float point[3])  { set (point); }

      bool         operator! () const        { return (isnan(p[0]) || isnan(p[1]) || isnan(p[2])); }
      bool         valid () const            { return (!(isnan(p[0]) || isnan(p[1]) || isnan(p[2]))); }

      float*       get ()                    { return (p); }
      const float* get () const              { return (p); }

      void         set (float x, float y, float z)      { p[0] = x; p[1] = y; p[2] = z; }
      void         set (const float point[3])           { memcpy (p, point, sizeof(p)); }
      void         zero ()                              { memset (p, 0, sizeof(p)); }

      float        norm2 () const                       { return (p[0]*p[0] + p[1]*p[1] + p[2]*p[2]); }
      float        norm () const                        { return (sqrt (norm2())); }
      Point&       normalise ()                         { Math::normalise (p); return (*this); }

      float&       operator[] (int idx)                 { return (p[idx]); }
      const float& operator[] (int idx) const           { return (p[idx]); }

      bool         operator== (const Point& A) const    { return (memcmp (p, A.p, sizeof(p)) == 0); };
      bool         operator!= (const Point& A) const    { return (memcmp (p, A.p, sizeof(p))); };
      Point        operator- () const                   { return (Point (-p[0], -p[1], -p[2])); }
      Point        operator* (float M) const            { return (Point (p[0]*M, p[1]*M, p[2]*M)); }

      Point&       operator= (const Point& A)           { memcpy (p, A.p, sizeof(p)); return (*this); }
      Point&       operator+= (const Point& inc)        { p[0] += inc[0]; p[1] += inc[1]; p[2] += inc[2]; return (*this); }
      Point&       operator-= (const Point& inc)        { p[0] -= inc[0]; p[1] -= inc[1]; p[2] -= inc[2]; return (*this); }
      Point&       operator*= (float M)                 { p[0] *= M; p[1] *= M; p[2] *= M; return (*this); }

      float        dot (const Point& A) const           { return (p[0]*A[0] + p[1]*A[1] + p[2]*A[2]); }
      Point        cross (const Point& A)               { return (Point (p[1]*A[2]-p[2]*A[1], p[2]*A[0]-p[0]*A[2], p[0]*A[1]-p[1]*A[0])); }

      void         invalidate ()                        { set (NAN, NAN, NAN); }
      static Point Invalid;

    protected:
      float p[3];
  };


  inline Point operator* (float M, const Point& P)        { return (Point (P[0]*M, P[1]*M, P[2]*M)); }
  inline Point operator+ (const Point& A, const Point& B) { return (Point (A[0]+B[0], A[1]+B[1], A[2]+B[2])); }
  inline Point operator- (const Point& A, const Point& B) { return (Point (A[0]-B[0], A[1]-B[1], A[2]-B[2])); }

  inline float dist2 (const Point& a, const Point& b) { return ((a-b).norm2()); }
  inline float dist (const Point& a, const Point& b) { return ((a-b).norm()); }

  inline std::ostream& operator<< (std::ostream& stream , const Point& pt)
  {
    stream << "[ " << pt[0] << " " << pt[1] << " " << pt[2] << " ]";
    return (stream);
  }



}

#endif


