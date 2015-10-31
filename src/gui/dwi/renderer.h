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

    29-10-2015 Robert E. Smith <robert.smith@florey.edu.au>
    * added support for dixel-based ODFs

*/

#ifndef __gui_dwi_renderer_h__
#define __gui_dwi_renderer_h__

#include "dwi/directions/set.h"
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

          typedef Eigen::MatrixXf matrix_t;
          typedef Eigen::VectorXf vector_t;

        public:
          Renderer () : is_SH (true), reverse_ID (0), origin_ID (0) { }

          bool ready () const { return shader_program; }

          void initGL () {
            sh.initGL();
            dixel.initGL();
            compile_shader();
          }

          void set_mode (const bool set_SH) {
            if (set_SH == is_SH)
              return;
            is_SH = set_SH;
            compile_shader();
          }

          void start (const Projection& projection, const GL::Lighting& lighting, float scale, 
              bool use_lighting, bool color_by_direction, bool hide_neg_lobes, bool orthographic = false);

          void draw (const Eigen::Vector3f& origin, int buffer_ID = 0) const {
            (void) buffer_ID; // to silence unused-parameter warnings
            gl::Uniform3fv (origin_ID, 1, origin.data());
            gl::Uniform1i (reverse_ID, 0);
            half_draw();
            gl::Uniform1i (reverse_ID, 1);
            half_draw();
          }

          void stop () const {
            shader_program.stop();
          }


        protected:
          bool is_SH;
          GL::Shader::Program shader_program;
          mutable GLuint reverse_ID, origin_ID;

          void compile_shader();
          std::string vertex_shader_source() const;
          std::string fragment_shader_source() const;

          void half_draw() const
          {
            if (is_SH)
              gl::DrawElements (gl::TRIANGLES, sh.num_indices(), gl::UNSIGNED_INT, (void*)0);
            else
              gl::DrawArrays (gl::TRIANGLES, 0, dixel.num_indices());
          }


        public:
          class SH
          {
            public:
              SH () { }

              void initGL();
              void bind() { half_sphere.vertex_buffer.bind (gl::ARRAY_BUFFER); VAO.bind(); half_sphere.index_buffer.bind(); }

              void update_mesh (const size_t LOD, const int lmax);

              void set_data (const vector_t& r_del_daz, int buffer_ID = 0) const {
                (void) buffer_ID; // to silence unused-parameter warnings
                surface_buffer.bind (gl::ARRAY_BUFFER);
                gl::BufferData (gl::ARRAY_BUFFER, r_del_daz.size()*sizeof(float), &r_del_daz[0], gl::STREAM_DRAW);
                gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 3*sizeof(GLfloat), (void*)0);
              }

              void compute_r_del_daz (matrix_t& r_del_daz, const matrix_t& SH) const {
                if (!SH.rows() || !SH.cols()) return;
                assert (transform.rows());
                r_del_daz.noalias() = SH * transform.transpose();
              }

              void compute_r_del_daz (vector_t& r_del_daz, const vector_t& SH) const {
                if (!SH.size()) return;
                assert (transform.rows());
                r_del_daz.noalias() = transform * SH;
              }

              GLuint num_indices() const { return half_sphere.num_indices; }

            private:
              matrix_t transform;
              Shapes::HalfSphere half_sphere;
              GL::VertexBuffer surface_buffer;
              GL::VertexArrayObject VAO;

              void update_transform (const std::vector<Shapes::HalfSphere::Vertex>&, int);

          } sh;


          class Dixel
          {
              typedef MR::DWI::Directions::dir_t dir_t;
            public:
              Dixel () { }

              void initGL();
              void bind() { vertex_buffer.bind (gl::ARRAY_BUFFER); VAO.bind(); }

              void update_mesh (const MR::DWI::Directions::Set&);

              void set_data (const vector_t&, int buffer_ID = 0) const;

              GLuint num_indices() const { return 3*polygons.size(); }

            private:
              GL::VertexBuffer vertex_buffer, value_buffer, normal_buffer;
              GL::VertexArrayObject VAO;

              void update_dixels (const MR::DWI::Directions::Set&);

              // TODO Could potentially compress the below code...
              //   Separate value from vertex direction in the shader?
              //   - Think this would then require normal calculation in a geometry shader

              // Need this in order to calculate normals on CPU
              std::vector<Eigen::Vector3f> directions;

              // Store the details for each polygon
              class Polygon
              {
                public:
                  Polygon (const dir_t i, const dir_t j, const dir_t k, const dir_t r) :
                      voxels { i, j, k },
                      dir_to_reverse (r) { }
                  dir_t operator[] (const size_t i) const { assert (i < 3); return voxels[i]; }
                  float reverse (const size_t i) const { assert (i < 3); return (dir_to_reverse == i ? -1.0f : 1.0f); }
                private:
                  const std::array<dir_t,3> voxels;
                  const dir_t dir_to_reverse;
              };
              std::vector<Polygon> polygons;

          } dixel;


      };


    }
  }
}

#endif

