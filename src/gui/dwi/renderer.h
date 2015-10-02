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

#include "gui/shapes/halfsphere.h"
#include "gui/opengl/shader.h"
#include "math/SH.h"

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
          typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> matrix_t;
          typedef Eigen::Matrix<float, Eigen::Dynamic, 1> vector_t;
        public:
          Renderer () : reverse_ID (0), origin_ID (0) { }

          bool ready () const { return shader_program; }
          void initGL ();
          void update_mesh (const size_t LOD, const int lmax);

          void compute_r_del_daz (matrix_t& r_del_daz, const matrix_t& SH) const {
            if (!SH.rows() || !SH.cols()) return;
            r_del_daz.noalias() = SH * transform.transpose();
          }

          void compute_r_del_daz (vector_t& r_del_daz, const vector_t& SH) const {
            if (!SH.size()) return;
            r_del_daz.noalias() = transform * SH;
          }


          void start (const Projection& projection, const GL::Lighting& lighting, float scale, 
              bool use_lighting, bool color_by_direction, bool hide_neg_lobes, bool orthographic = false) const;

          void set_data (const vector_t& r_del_daz, int buffer_ID = 0) const {
            (void) buffer_ID; // to silence unused-parameter warnings
            surface_buffer.bind (gl::ARRAY_BUFFER);
            gl::BufferData (gl::ARRAY_BUFFER, r_del_daz.size()*sizeof(float), &r_del_daz[0], gl::STREAM_DRAW);
            gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 3*sizeof(GLfloat), (void*)0);
          }

          void draw (const Eigen::Vector3f& origin, int buffer_ID = 0) const {
            (void) buffer_ID; // to silence unused-parameter warnings
            gl::Uniform3fv (origin_ID, 1, origin.data());
            gl::Uniform1i (reverse_ID, 0);
            gl::DrawElements (gl::TRIANGLES, half_sphere.num_indices, gl::UNSIGNED_INT, (void*)0);
            gl::Uniform1i (reverse_ID, 1);
            gl::DrawElements (gl::TRIANGLES, half_sphere.num_indices, gl::UNSIGNED_INT, (void*)0);
          }
          void stop () const {
            shader_program.stop();
          }

        protected:

          void update_transform (const std::vector<Shapes::HalfSphere::Vertex>& vertices, int lmax);

          matrix_t transform;

          GL::Shader::Program shader_program;
          Shapes::HalfSphere half_sphere;
          GL::VertexBuffer surface_buffer;
          GL::VertexArrayObject vertex_array_object;
          mutable GLuint reverse_ID, origin_ID;
      };


    }
  }
}

#endif

