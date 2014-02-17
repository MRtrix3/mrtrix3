#ifndef __gui_dwi_renderer_h__
#define __gui_dwi_renderer_h__

#include "gui/opengl/shader.h"
#include "math/matrix.h"
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
              bool use_lighting, bool color_by_direction, bool hide_neg_lobes, bool orthographic = false) const;

          void set_data (const Math::Vector<float>& r_del_daz, int buffer_ID = 0) const {
            assert (r_del_daz.stride() == 1);
            surface_buffer.bind (gl::ARRAY_BUFFER);
            gl::BufferData (gl::ARRAY_BUFFER, r_del_daz.size()*sizeof(float), &r_del_daz[0], gl::STREAM_DRAW);
            gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 3*sizeof(GLfloat), (void*)0);
          }

          void draw (const float* origin, int buffer_ID = 0) const {
            gl::Uniform3fv (origin_ID, 1, origin);
            gl::Uniform1i (reverse_ID, 0);
            gl::DrawElements (gl::TRIANGLES, num_indices, gl::UNSIGNED_INT, (void*)0);
            gl::Uniform1i (reverse_ID, 1);
            gl::DrawElements (gl::TRIANGLES, num_indices, gl::UNSIGNED_INT, (void*)0);
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

