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

#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace GL
    {




      class vec4 {
        public:
          vec4 () { }
          vec4 (float x, float y, float z, float w) { v[0] = x; v[1] = y; v[2] = z; v[3] = w; }
          vec4 (const Eigen::Quaternionf& q) { v[0] = q.x(); v[1] = q.y(); v[2] = q.z(); v[3] = q.w(); }
          vec4 (const Eigen::Vector3& p, float w) { v[0] = p[0]; v[1] = p[1]; v[2] = p[2]; v[3] = w;  }
          vec4 (const float* p) { memcpy (v, p, sizeof(v)); }

          void zero () {
            memset (v, 0, sizeof (v));
          }

          operator const GLfloat* () const { return v; }
          operator GLfloat* () { return v; }

          friend std::ostream& operator<< (std::ostream& stream, const vec4& v) {
            for (size_t i = 0; i < 4; ++i) 
              stream << v[i] << " ";
            return stream;
          }

        protected:
          GLfloat v[4];
      };





      class mat4 {
        public:
          mat4 () { } 
          mat4 (const mat4& a) { memcpy (m, a.m, sizeof(m)); }
          mat4 (const float* p) { memcpy (m, p, sizeof(m)); }
          mat4 (const Eigen::Quaternionf& q)
          {
            const Eigen::Quaternionf::Matrix3 R = q.matrix();
            zero();
            for (size_t i = 0; i != 3; ++i) {
              for (size_t j = 0; j != 3; ++j)
                (*this)(i,j) = R(i,j);
            }
            (*this)(3,3) = 1.0f;
          }
          template <class M>
          mat4 (const M& m)
          {
            for (size_t i = 0; i != 4; ++i) {
              for (size_t j = 0; j != 4; ++j)
                (*this)(i,j) = m(i,j);
            }
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

          vec4 operator* (const vec4& v) const {
            vec4 r;
            r.zero();
            for (size_t j = 0; j < 4; ++j) 
              for (size_t i = 0; i < 4; ++i) 
                r[i] += (*this)(i,j) * v[j];
            return r;
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
        Eigen::Matrix<float, 4, 4> A;
        for (size_t i = 0; i != 4; ++i) {
          for (size_t j = 0; j != 4; ++j)
            A(i,j) = a(i,j);
        }
        return mat4 (A.inverse().eval());
      }



      inline mat4 ortho (float L, float R, float B, float T, float N, float F) 
      {
        mat4 m;
        m.zero();
        m(0,0) = 2.0f/(R-L);
        m(1,1) = 2.0f/(T-B);
        m(2,2) = 2.0f/(N-F);
        m(3,3) = 1.0f;

        m(0,3) = (R+L)/(R-L);
        m(1,3) = (T+B)/(T-B);
        m(2,3) = (F+N)/(F-N);

        return m;
      }



      inline mat4 frustum (float L, float R, float B, float T, float N, float F) 
      {
        mat4 m;
        m.zero();

        m(0,0) = 2.0f*N/(R-L);
        m(1,1) = 2.0f*N/(T-B);
        m(0,2) = (R+L)/(R-L);
        m(1,2) = (T+B)/(T-B);
        m(2,2) = (F+N)/(N-F);
        m(3,2) = -1.0f;
        m(2,3) = 2.0f*F*N/(N-F);

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

      template <typename ValueType>
      inline mat4 translate (const Eigen::Vector3& x)
      {
        return translate (x[0], x[1], x[2]);
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

