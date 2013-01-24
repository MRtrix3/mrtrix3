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


    24-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * added support of overlay of orientation plot on main window

*/

#ifndef __gui_dwi_renderer_h__
#define __gui_dwi_renderer_h__

#include "math/matrix.h"
#include "math/SH.h"
#include "gui/opengl/shader.h"

namespace MR
{
  namespace GUI
  {
    namespace DWI
    {

      class Renderer
      {
        public:
          Renderer () :
            lmax_computed (0), lod_computed (0), hide_neg_lobes (true), recompute_amplitudes (true),
            recompute_mesh (true) { }

          bool ready () const {
            return shader_program;
          }
          void init ();
          void set_values (const std::vector<float>& values) {
            SH.allocate (values.size());
            for (size_t n = 0; n < SH.size(); ++n)
              SH[n] = values[n];
            recompute_amplitudes = true;
          }
          void scale_values (float factor) {
            for (size_t n = 0; n < SH.size(); ++n)
              SH[n] *= factor;
            recompute_amplitudes = true;
          }
          void set_hide_neg_lobes (bool hide) {
            hide_neg_lobes = hide;
          }
          void set_lmax (int lmax) {
            if (lmax != lmax_computed) recompute_mesh = true;
            lmax_computed = lmax;
          }
          void set_LOD (int lod) {
            if (lod != lod_computed) recompute_mesh = true;
            lod_computed = lod;
          }
          void draw (float scale, bool use_normals, const float* colour = NULL);

          const Math::Vector<float>& get_values () const { return SH; }
          int get_LOD () const { return lod_computed; }
          int get_lmax () const { return lmax_computed; }
          bool get_hide_neg_lobes () const { return hide_neg_lobes; }

          size_t size () const { return transform.rows(); }
          bool empty () const { return transform.rows() == 0; }

        protected:

          class Vertex {
            public:
              Vertex () { }
              Vertex (const float x[3]) { p[0] = x[0]; p[1] = x[1]; p[2] = x[2]; }
              Vertex (const std::vector<Vertex>& vertices, size_t i1, size_t i2) {
                p[0] = vertices[i1][0] + vertices[i2][0];
                p[1] = vertices[i1][1] + vertices[i2][1];
                p[2] = vertices[i1][2] + vertices[i2][2];
                Math::normalise (p); 
              }

              float& operator[] (int n) { return p[n]; }
              float operator[] (int n) const { return p[n]; }

            private:
              float p[3];
          };


          class Triangle
          {
            public:
              Triangle () { }
              Triangle (const uint32_t x[3]) {
                index[0] = x[0];
                index[1] = x[1];
                index[2] = x[2];
              }
              Triangle (size_t i1, size_t i2, size_t i3) {
                index[0] = i1;
                index[1] = i2;
                index[2] = i3;
              }
              void set (size_t i1, size_t i2, size_t i3) {
                index[0] = i1;
                index[1] = i2;
                index[2] = i3;
              }
              GLuint& operator[] (int n) {
                return index[n];
              }
            protected:
              uint32_t  index[3];
          };

          class Edge
          {
            public:
              Edge (const Edge& E) {
                set (E.i1, E.i2);
              }
              Edge (uint32_t a, uint32_t b) {
                set (a,b);
              }
              bool operator< (const Edge& E) const {
                return (i1 < E.i1 ? true : i2 < E.i2);
              }
              void set (uint32_t a, uint32_t b) {
                if (a < b) {
                  i1 = a;
                  i2 = b;
                }
                else {
                  i1 = b;
                  i2 = a;
                }
              }
              uint32_t i1;
              uint32_t i2;
          };

          void compute_amplitudes ();
          void compute_mesh ();
          void compute_transform ();
/*
          GLfloat* get_r (GLfloat* row) {
            return row+3;
          }
          GLfloat* get_daz (GLfloat* row) {
            return row+3+nsh;
          }
          GLfloat* get_del (GLfloat* row) {
            return row+3+2*nsh;
          }
*/
          std::vector<Vertex> vertices;
          std::vector<Triangle> indices;
          std::vector<GLfloat> amplitudes_and_derivatives;
          Math::Matrix<float> transform;

          Math::Vector<float> SH;
          int lmax_computed, lod_computed;
          bool hide_neg_lobes, recompute_amplitudes, recompute_mesh;
          //void   precompute_row (GLfloat row);
          GL::Shader::Program shader_program;
      };


    }
  }
}

#endif

