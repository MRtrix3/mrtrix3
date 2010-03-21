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

#ifndef __dwi_renderer_h__
#define __dwi_renderer_h__

#include "math/matrix.h"
#include "opengl/shader.h"
#include "math/SH.h"

namespace MR {
  namespace DWI {

    class Renderer 
    {
      public:
        Renderer () : 
          lmax_computed (0), lod_computed (0), hide_neg_lobes (true), recalculate (true),
          recompute (true), nsh (0), row_size (0) { }
        ~Renderer () { clear(); }

        bool ready () const { return (shader_program); }
        void init ();
        void set_values (const std::vector<float>& values) { SH = values; recalculate = true; }
        void set_hide_neg_lobes (bool hide) { hide_neg_lobes = hide; }
        void set_lmax (int lmax) { if (lmax != lmax_computed) recompute = true; lmax_computed = lmax; }
        void set_LOD (int lod) { if (lod != lod_computed) recompute = true; lod_computed = lod; }
        void draw (bool use_normals, const float* colour = NULL);

        int get_LOD () const { return (lod_computed); }
        int get_lmax () const { return (lmax_computed); }
        bool get_hide_neg_lobes () const { return (hide_neg_lobes); }

        size_t size () const { return (rows.size()); }
        bool empty () const { return (rows.empty()); }

      protected:
        class Vertex {
          public:
            GLfloat P[3];
            GLfloat N[3];
            GLubyte C[3];
        };

        class Triangle {
          public:
            Triangle () { } 
            Triangle (size_t i1, size_t i2, size_t i3) { index[0] = i1; index[1] = i2; index[2] = i3; }
            void set (size_t i1, size_t i2, size_t i3) { index[0] = i1; index[1] = i2; index[2] = i3; }
            GLuint& operator[] (int n) { return (index[n]); }
          protected:
            GLuint  index[3];
        };

        class Edge {
          public:
            Edge (const Edge& E) { set (E.i1, E.i2); }
            Edge (size_t a, size_t b) { set (a,b); }
            bool operator< (const Edge& E) const { return (i1 < E.i1 ? true : i2 < E.i2); }
            void set (size_t a, size_t b) { if (a < b) { i1 = a; i2 = b; } else { i1 = b; i2 = a; } }
            size_t i1;
            size_t i2;
        };

        void calculate ();
        void precompute ();
        void clear () { for (std::vector<GLfloat*>::iterator i = rows.begin(); i != rows.end(); ++i) delete [] *i; rows.clear(); }

        GLfloat* get_r (GLfloat* row) { return (row+3); }
        GLfloat* get_daz (GLfloat* row) { return (row+3+nsh); }
        GLfloat* get_del (GLfloat* row) { return (row+3+2*nsh); }

        GLfloat* push_back (GLfloat* p) { 
          GLfloat* row = new GLfloat [row_size];
          row[0] = p[0];
          row[1] = p[1];
          row[2] = p[2];
          precompute_row (row);
          return (row);
        }

        GLfloat* push_back (size_t i1, size_t i2) { 
          GLfloat* row = new GLfloat [row_size];
          const GLfloat* p1 (rows[i1]);
          const GLfloat* p2 (rows[i2]);
          row[0] = p1[0] + p2[0];
          row[1] = p1[1] + p2[1];
          row[2] = p1[2] + p2[2];
          precompute_row (row);
          return (row);
        }

        std::vector<Vertex> vertices;
        std::vector<Triangle> indices;
        std::vector<GLfloat*> rows;

        std::vector<float> SH;
        int lmax_computed, lod_computed;
        bool hide_neg_lobes, recalculate, recompute;
        size_t  nsh, row_size;
        void   precompute_row (GLfloat* row); 
        //GLhandleARB vertex_shader, fragment_shader, shader_program;
        GL::Shader::Vertex vertex_shader;
        GL::Shader::Fragment fragment_shader;
        GL::Shader::Program shader_program;
    };


  }
}

#endif

