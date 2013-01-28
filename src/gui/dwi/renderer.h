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
          Renderer () :
            num_indices (0), lmax_computed (0), lod_computed (0), recompute_amplitudes (true),
            recompute_mesh (true), vertex_buffer_ID (0), surface_buffer_ID (0), 
            index_buffer_ID (0), vertex_array_object_ID (0) { }

          ~Renderer ();

          bool ready () const {
            return shader_program;
          }
          void init ();
          void set_values (const Math::Vector<float>& values) {
            SH = values;
            recompute_amplitudes = true;
          }
          void set_lmax (int lmax) {
            if (lmax != lmax_computed) recompute_mesh = true;
            lmax_computed = lmax;
          }
          void set_LOD (int lod) {
            if (lod != lod_computed) recompute_mesh = true;
            lod_computed = lod;
          }
          void setupGL (const Projection& projection, const GL::Lighting& lighting, float scale, 
              bool use_lighting, bool color_by_direction, bool hide_neg_lobes);
          void draw (float* origin, int n = 0) {
            glUniform3fv (origin_ID, 1, origin);
            glUniform1i (reverse_ID, 0);
            glDrawElements (GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (void*)0);
            glUniform1i (reverse_ID, 1);
            glDrawElements (GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (void*)0);
          }
          void doneGL () {
            shader_program.stop();
          }
          
          void draw (const Projection& projection, const GL::Lighting& lighting, float* origin, float scale, 
              bool use_lighting, bool color_by_direction, bool hide_neg_lobes) {
            if (recompute_mesh) compute_mesh(); 
            if (recompute_amplitudes) compute_amplitudes(); 
            setupGL (projection, lighting, scale, use_lighting, color_by_direction, hide_neg_lobes);
            draw (origin);
            doneGL();
          }



          const Math::Vector<float>& get_values () const { return SH; }
          int get_LOD () const { return lod_computed; }
          int get_lmax () const { return lmax_computed; }

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

          void compute_amplitudes ();
          void compute_mesh ();
          void compute_transform (const std::vector<Vertex>& vertices);

          Math::Matrix<float> transform;

          Math::Vector<float> SH;
          size_t num_indices;
          int lmax_computed, lod_computed;
          bool recompute_amplitudes, recompute_mesh;
          GL::Shader::Program shader_program;
          GLuint vertex_buffer_ID, surface_buffer_ID, index_buffer_ID, vertex_array_object_ID, reverse_ID, origin_ID;
      };


    }
  }
}

#endif

