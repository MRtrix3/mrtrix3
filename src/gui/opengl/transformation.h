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

#ifndef __gui_opengl_transformation_h__
#define __gui_opengl_transformation_h__

#include <iostream>

#include "math/LU.h"
#include "math/versor.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace GL
    {

      class mat4 {
        public:
          mat4 () { } 
          mat4 (const mat4& a) { memcpy (m, a.m, sizeof(m)); }
          mat4 (const float* p) { memcpy (m, p, sizeof(m)); }
          template <typename T>
            mat4 (const Math::Versor<T>& Q) {
              zero();
              (*this)(0,0) = Q[0]*Q[0] + Q[1]*Q[1] - Q[2]*Q[2] - Q[3]*Q[3];
              (*this)(0,1) = 2.0f*Q[1]*Q[2] - 2.0f*Q[0]*Q[3];
              (*this)(0,2) = 2.0f*Q[1]*Q[3] + 2.0f*Q[0]*Q[2];

              (*this)(1,0) = 2.0f*Q[1]*Q[2] + 2.0f*Q[0]*Q[3];
              (*this)(1,1) = Q[0]*Q[0] + Q[2]*Q[2] - Q[1]*Q[1] - Q[3]*Q[3];
              (*this)(1,2) = 2.0f*Q[2]*Q[3] - 2.0f*Q[0]*Q[1];

              (*this)(2,0) = 2.0f*Q[1]*Q[3] - 2.0f*Q[0]*Q[2];
              (*this)(2,1) = 2.0f*Q[2]*Q[3] + 2.0f*Q[0]*Q[1];
              (*this)(2,2) = Q[0]*Q[0] + Q[3]*Q[3] - Q[2]*Q[2] - Q[1]*Q[1];
            }

          void zero () {
            memset (m, 0, sizeof (m));
          }

          operator const GLfloat* () const { return m; }
          operator GLfloat* () { return m; }

          GLfloat& operator() (size_t i, size_t j) { return m[i+4*j]; }
          const GLfloat& operator() (size_t i, size_t j) const { return m[i+4*j]; }

          mat4 operator* (const mat4& a) const {
            mat4 t;
            for (size_t j = 0; j < 4; ++j) {
              for (size_t i = 0; i < 4; ++i) {
                t(i,j) = 0.0f;
                for (size_t k = 0; k < 4; ++k)
                  t(i,j) += (*this)(i,k) * a(k,j);

              }
            }
            return t;
          }

          mat4& operator*= (const mat4& a) {
            *this = (*this) * a;
            return *this;
          }

          friend std::ostream& operator<< (std::ostream& stream, const mat4& m) {
            for (size_t i = 0; i < 4; ++i) {
              for (size_t j = 0; j < 4; ++j) 
                stream << m(i,j) << " ";
              stream << "\n";
            }
            return stream;
          }


        protected:
          GLfloat m[16];
      };




      inline mat4 identity () {
        mat4 m;
        m.zero();
        m(0,0) = m(1,1) = m(2,2) = m(3,3) = 1.0f;
        return m;
      }



      inline mat4 transpose (const mat4& a) 
      {
        mat4 b;
        for (size_t j = 0; j < 4; ++j)
          for (size_t i = 0; i < 4; ++i)
            b(i,j) = a(j,i);
        return b;
      }





      inline mat4 inv (const mat4& a) 
      {
        mat4 b;
        Math::Matrix<float> A (const_cast<float*> (&a(0,0)), 4, 4); 
        Math::Matrix<float> B (&b(0,0), 4, 4); 
        Math::LU::inv (B, A);
        return b;
      }



      inline mat4 ortho (float left, float right, float bottom, float top, float near, float far) 
      {
        mat4 m;
        m.zero();
        m(0,0) = 2.0f/(right-left);
        m(1,1) = 2.0f/(top-bottom);
        m(2,2) = 2.0f/(near-far);
        m(3,3) = 1.0f;

        m(0,3) = (right+left)/(right-left);
        m(1,3) = (top+bottom)/(top-bottom);
        m(2,3) = (far+near)/(far-near);

        return m;
      }



      inline mat4 frustum (float left, float right, float bottom, float top, float near, float far) 
      {
        mat4 m;
        m.zero();

        m(0,0) = 2.0f*near/(right-left);
        m(1,1) = 2.0f*near/(top-bottom);
        m(0,2) = (right+left)/(right-left);
        m(1,2) = (top+bottom)/(top-bottom);
        m(2,2) = (far+near)/(near-far);
        m(3,2) = -1.0f;
        m(2,3) = 2.0f*far*near/(near-far);

        return m;
      }


      inline mat4 translate (float x, float y, float z) 
      {
        mat4 m = identity();
        m(0,3) = x;
        m(1,3) = y;
        m(2,3) = z;
        m(3,3) = 1.0f;
        return m;
      }


      inline mat4 scale (float x, float y, float z) 
      {
        mat4 m;
        m.zero();
        m(0,0) = x;
        m(1,1) = z;
        m(2,2) = y;
        m(3,3) = 1.0f;
        return m;
      }

      inline mat4 scale (float s) { return scale (s,s,s); }

    }
  }
}

#endif

