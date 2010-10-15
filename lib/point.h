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

  template <typename T = float> class Point 
  {
    public:
      typedef T value_type;

      Point () { invalidate (); }
      Point (value_type x, value_type y, value_type z) { set (x, y, z); }
      template <typename U> Point (const U point[3]) { set (point); }
      template <typename U> Point (const Point<U>& A) { set (A[0], A[1], A[2]); }

      bool operator! () const { return (isnan(p[0]) || isnan(p[1]) || isnan(p[2])); }
      bool valid ()     const { return (!(isnan(p[0]) || isnan(p[1]) || isnan(p[2]))); }

      value_type*       get ()       { return (p); }
      const value_type* get () const { return (p); }

      void set (value_type x, value_type y, value_type z) { p[0] = x; p[1] = y; p[2] = z; }
      template <typename U> void set (const U point[3]) { set (point[0], point[1], point[2]); }
      void zero () { memset (p, 0, sizeof(p)); }

      value_type norm2 () const { return (p[0]*p[0] + p[1]*p[1] + p[2]*p[2]); }
      value_type norm () const  { return (sqrt (norm2())); }
      Point&     normalise ()   { Math::normalise (p); return (*this); }

      value_type&       operator[] (int idx)                 { return (p[idx]); }
      const value_type& operator[] (int idx) const           { return (p[idx]); }

      bool         operator== (const Point& A) const    { return (memcmp (p, A.p, sizeof(p)) == 0); };
      bool         operator!= (const Point& A) const    { return (memcmp (p, A.p, sizeof(p))); };
      Point        operator- () const                   { return (Point (-p[0], -p[1], -p[2])); }
      Point operator* (value_type M) const { return (Point (p[0]*M, p[1]*M, p[2]*M)); }

      template <typename U> Point& operator=  (const Point<U>& A)   { set (A[0], A[1], A[2]); return (*this); }
      template <typename U> Point& operator+= (const Point<U>& inc) { p[0] += inc[0]; p[1] += inc[1]; p[2] += inc[2]; return (*this); }
      template <typename U> Point& operator-= (const Point<U>& inc) { p[0] -= inc[0]; p[1] -= inc[1]; p[2] -= inc[2]; return (*this); }
      template <typename U> Point& operator*= (U M)                 { p[0] *= M; p[1] *= M; p[2] *= M; return (*this); }
      template <typename U> Point& operator/= (U M)                 { p[0] /= M; p[1] /= M; p[2] /= M; return (*this); }

      value_type   dot (const Point& A) const   { return (p[0]*A[0] + p[1]*A[1] + p[2]*A[2]); }
      Point        cross (const Point& A) const { return (Point (p[1]*A[2]-p[2]*A[1], p[2]*A[0]-p[0]*A[2], p[0]*A[1]-p[1]*A[0])); }

      void invalidate () { set (value_type (NAN), value_type (NAN), value_type (NAN)); }

    protected:
      value_type p[3];
  };


  template <typename T> inline Point<T> operator* (T M, const Point<T>& P) { return (Point<T> (P[0]*M, P[1]*M, P[2]*M)); }
  template <typename T> inline Point<T> operator+ (const Point<T>& A, const Point<T>& B) { return (Point<T> (A[0]+B[0], A[1]+B[1], A[2]+B[2])); }

  template <typename T> inline Point<T> operator- (const Point<T>& A, const Point<T>& B) { return (Point<T> (A[0]-B[0], A[1]-B[1], A[2]-B[2])); }

  template <typename T> inline T dist2 (const Point<T>& a, const Point<T>& b) { return ((a-b).norm2()); }
  template <typename T> inline T dist (const Point<T>& a, const Point<T>& b) { return ((a-b).norm()); }

  template <typename T> inline std::ostream& operator<< (std::ostream& stream , const Point<T>& pt)
  {
    stream << "[ " << pt[0] << " " << pt[1] << " " << pt[2] << " ]";
    return (stream);
  }



}

#endif


