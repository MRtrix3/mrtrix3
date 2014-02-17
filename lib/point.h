/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __point_h__
#define __point_h__

#include "mrtrix.h"
#include "math/vector.h"

namespace MR
{

  template <typename T = float> class Point
  {
    public:
      typedef T value_type;

      Point () {
        invalidate ();
      }
      Point (value_type x, value_type y, value_type z) {
        set (x, y, z);
      }
      template <typename U> Point (const U point[3]) {
        set (point);
      }
      template <typename U> Point (const Point<U>& A) {
        set (A[0], A[1], A[2]);
      }
      template <class U> Point (const U& A) {
        assert (A.size() == 3);
        set (A[0], A[1], A[2]);
      }

      bool operator! () const {
        return (isnan (p[0]) || isnan (p[1]) || isnan (p[2]));
      }
      bool valid ()     const {
        return (! (isnan (p[0]) || isnan (p[1]) || isnan (p[2])));
      }

      operator value_type* () { 
        return (p);
      }
      operator const value_type* () const {
        return (p);
      }

      void set (value_type x, value_type y, value_type z) {
        p[0] = x;
        p[1] = y;
        p[2] = z;
      }
      template <typename U> void set (const U point[3]) {
        set (point[0], point[1], point[2]);
      }
      void zero () {
        memset (p, 0, sizeof (p));
      }

      value_type norm2 () const {
        return (p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
      }
      value_type norm () const  {
        return (sqrt (norm2()));
      }
      Point&     normalise ()   {
        Math::normalise (p);
        return (*this);
      }

      value_type&       operator[] (int idx)                 {
        return (p[idx]);
      }
      const value_type& operator[] (int idx) const           {
        return (p[idx]);
      }

      bool         operator== (const Point& A) const    {
        return (memcmp (p, A.p, sizeof (p)) == 0);
      };
      bool         operator!= (const Point& A) const    {
        return (memcmp (p, A.p, sizeof (p)));
      };
      Point        operator- () const                   {
        return (Point (-p[0], -p[1], -p[2]));
      }
      Point operator* (value_type M) const {
        return (Point (p[0]*M, p[1]*M, p[2]*M));
      }
      Point operator/ (value_type M) const {
        return (Point (p[0]/M, p[1]/M, p[2]/M));
      }

      template <typename U> Point& operator= (const Point<U>& A)   {
        set (A[0], A[1], A[2]);
        return (*this);
      }
      template <typename U> Point& operator+= (const Point<U>& inc) {
        p[0] += inc[0];
        p[1] += inc[1];
        p[2] += inc[2];
        return (*this);
      }
      template <typename U> Point& operator-= (const Point<U>& inc) {
        p[0] -= inc[0];
        p[1] -= inc[1];
        p[2] -= inc[2];
        return (*this);
      }
      template <typename U> Point& operator*= (U M)                 {
        p[0] *= M;
        p[1] *= M;
        p[2] *= M;
        return (*this);
      }
      template <typename U> Point& operator/= (U M)                 {
        p[0] /= M;
        p[1] /= M;
        p[2] /= M;
        return (*this);
      }

      bool operator< (const Point<value_type>& a) const {
        if (p[2] == a[2]) {
          if (p[1] == a[1])
            return p[0] < a[0];
          return p[1] < a[1];
        }
        return p[2] < a[2];
      }

      value_type   dot (const Point& A) const   {
        return (p[0]*A[0] + p[1]*A[1] + p[2]*A[2]);
      }
      Point        cross (const Point& A) const {
        return (Point (p[1]*A[2]-p[2]*A[1], p[2]*A[0]-p[0]*A[2], p[0]*A[1]-p[1]*A[0]));
      }

      void invalidate () {
        set (value_type (NAN), value_type (NAN), value_type (NAN));
      }

    protected:
      value_type p[3];
  };


  template <typename T> inline Point<T> operator* (T M, const Point<T>& P)
  {
    return (Point<T> (P[0]*M, P[1]*M, P[2]*M));
  }
  template <typename T> inline Point<T> operator+ (const Point<T>& A, const Point<T>& B)
  {
    return (Point<T> (A[0]+B[0], A[1]+B[1], A[2]+B[2]));
  }

  template <typename T> inline Point<T> operator- (const Point<T>& A, const Point<T>& B)
  {
    return (Point<T> (A[0]-B[0], A[1]-B[1], A[2]-B[2]));
  }

  template <typename T> inline T dist2 (const Point<T>& a, const Point<T>& b)
  {
    return ( (a-b).norm2());
  }
  template <typename T> inline T dist (const Point<T>& a, const Point<T>& b)
  {
    return ( (a-b).norm());
  }

  template <typename T> inline std::ostream& operator<< (std::ostream& stream , const Point<T>& pt)
  {
    stream << "[ " << pt[0] << " " << pt[1] << " " << pt[2] << " ]";
    return (stream);
  }



}

#endif


