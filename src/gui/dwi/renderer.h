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

    class Projection;

    namespace GL 
    {
      class Lighting;
    }

    namespace DWI
    {

      class Renderer
      {
        public:
          Renderer () : num_indices (0) { }

          bool ready () const { return shader_program; }
          void initGL ();
          void update_mesh (int LOD, int lmax);

          void compute_r_del_daz (Math::Matrix<float>& r_del_daz, const Math::Matrix<float>& SH) const {
            if (!SH.rows() || !SH.columns()) return;
            Math::mult (r_del_daz, 1.0f, CblasNoTrans, SH, CblasTrans, transform);
          }

          void compute_r_del_daz (Math::Vector<float>& r_del_daz, const Math::Vector<float>& SH) const {
            if (!SH.size()) return;
            Math::mult (r_del_daz, transform, SH);
          }


          void start (const Projection& projection, const GL::Lighting& lighting, float scale, 
              bool use_lighting, bool color_by_direction, bool hide_neg_lobes) const;

          void set_data (const Math::Vector<float>& r_del_daz, int buffer_ID = 0) const {
            assert (r_del_daz.stride() == 1);
            surface_buffer.bind (GL_ARRAY_BUFFER);
            glBufferData (GL_ARRAY_BUFFER, r_del_daz.size()*sizeof(float), &r_del_daz[0], GL_STREAM_DRAW);
            glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), (void*)0);
          }

          void draw (const float* origin, int buffer_ID = 0) const {
            glUniform3fv (origin_ID, 1, origin);
            glUniform1i (reverse_ID, 0);
            glDrawElements (GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (void*)0);
            glUniform1i (reverse_ID, 1);
            glDrawElements (GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (void*)0);
          }
          void stop () const {
            shader_program.stop();
          }

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

          void update_transform (const std::vector<Vertex>& vertices, int lmax);

          Math::Matrix<float> transform;

          size_t num_indices;
          GL::Shader::Program shader_program;
          GL::VertexBuffer vertex_buffer, surface_buffer, index_buffer;
          GL::VertexArrayObject vertex_array_object;
          mutable GLuint reverse_ID, origin_ID;
      };


    }
  }
}

#endif

