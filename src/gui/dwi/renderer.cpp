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

#include <map>

#include "gui/gui.h"
#include "gui/dwi/renderer.h"
#include "math/legendre.h"
#include "gui/projection.h"
#include "gui/opengl/lighting.h"



namespace MR
{
  namespace GUI
  {
    namespace DWI
    {




      void Renderer::start (const Projection& projection, const GL::Lighting& lighting, float scale, 
          bool use_lighting, bool color_by_direction, bool hide_neg_lobes, bool orthographic)
      {
        if (is_SH)
          sh.bind();
        else
          dixel.bind();
        shader_program.start();

        gl::UniformMatrix4fv (gl::GetUniformLocation (shader_program, "MV"), 1, gl::FALSE_, projection.modelview());
        gl::UniformMatrix4fv (gl::GetUniformLocation (shader_program, "MVP"), 1, gl::FALSE_, projection.modelview_projection());
        gl::Uniform3fv (gl::GetUniformLocation (shader_program, "light_pos"), 1, lighting.lightpos);
        gl::Uniform1f (gl::GetUniformLocation (shader_program, "ambient"), lighting.ambient);
        gl::Uniform1f (gl::GetUniformLocation (shader_program, "diffuse"), lighting.diffuse);
        gl::Uniform1f (gl::GetUniformLocation (shader_program, "specular"), lighting.specular);
        gl::Uniform1f (gl::GetUniformLocation (shader_program, "shine"), lighting.shine);
        gl::Uniform1f (gl::GetUniformLocation (shader_program, "scale"), scale);
        gl::Uniform1i (gl::GetUniformLocation (shader_program, "color_by_direction"), color_by_direction);
        gl::Uniform1i (gl::GetUniformLocation (shader_program, "use_lighting"), use_lighting);
        gl::Uniform1i (gl::GetUniformLocation (shader_program, "hide_neg_lobes"), hide_neg_lobes);
        gl::Uniform1i (gl::GetUniformLocation (shader_program, "orthographic"), orthographic);
        gl::Uniform3fv (gl::GetUniformLocation (shader_program, "constant_color"), 1, lighting.object_color);
        reverse_ID = gl::GetUniformLocation (shader_program, "reverse");
        origin_ID = gl::GetUniformLocation (shader_program, "origin");
      }



      void Renderer::compile_shader()
      {
        GL_CHECK_ERROR;
        if (shader_program)
          shader_program.clear();
        GL::Shader::Vertex vertex_shader (vertex_shader_source());
        GL::Shader::Fragment fragment_shader (fragment_shader_source());
        shader_program.attach (vertex_shader);
        shader_program.attach (fragment_shader);
        shader_program.link();
        GL_CHECK_ERROR;
      }



      std::string Renderer::vertex_shader_source() const
      {
        std::string source;

        if (is_SH) {
          source +=
          "layout(location = 0) in vec3 vertex;\n"
          "layout(location = 1) in vec3 r_del_daz;\n";
        } else {
          source +=
          "layout(location = 0) in vec3 vertex;\n"
          "layout(location = 1) in float value;\n"
          "layout(location = 2) in vec3 face_normal;\n";
        }

        source +=
          "uniform int color_by_direction, use_lighting, reverse, orthographic;\n"
          "uniform float scale;\n"
          "uniform vec3 constant_color, origin;\n"
          "uniform mat4 MV, MVP;\n"
          "out vec3 position, color, normal;\n"
          "out float amplitude;\n"
          "void main () {\n";

        if (is_SH) {
          source +=
          "  amplitude = r_del_daz[0];\n"
          "  if (use_lighting != 0) {\n"
          "    bool atpole = ( vertex.x == 0.0 && vertex.y == 0.0 );\n"
          "    float az = atpole ? 0.0 : atan (vertex.y, vertex.x);\n"
          "    float caz = cos (az), saz = sin (az), cel = vertex.z, sel = sqrt (1.0 - cel*cel);\n"
          "    vec3 d1;\n"
          "    if (atpole)\n"
          "      d1 = vec3 (-r_del_daz[0]*saz, r_del_daz[0]*caz, r_del_daz[2]);\n"
          "    else\n"
          "      d1 = vec3 (r_del_daz[2]*caz*sel - r_del_daz[0]*sel*saz, r_del_daz[2]*saz*sel + r_del_daz[0]*sel*caz, r_del_daz[2]*cel);\n"
          "    vec3 d2 = vec3 (-r_del_daz[1]*caz*sel - r_del_daz[0]*caz*cel,\n"
          "                    -r_del_daz[1]*saz*sel - r_del_daz[0]*saz*cel,\n"
          "                    -r_del_daz[1]*cel     + r_del_daz[0]*sel);\n"
          "    normal = cross (d1, d2);\n";
        } else {
          source +=
          "  amplitude = value;\n"
          "  if (use_lighting != 0) {\n"
          "    normal = face_normal;\n";
        }

        source +=
          "    if (reverse != 0)\n"
          "      normal = -normal;\n"
          "    normal = normalize (mat3(MV) * normal);\n"
          "  }\n"
          "  if (color_by_direction != 0)\n"
          "     color = abs (vertex.xyz);\n"
          "  else\n"
          "     color = constant_color;\n"
          "  vec3 pos = vertex * amplitude * scale;\n"
          "  if (reverse != 0)\n"
          "    pos = -pos;\n"
          "  if (orthographic != 0)\n"
          "    position = vec3(0.0, 0.0, 1.0);\n"
          "  else\n"
          "    position = -(MV * vec4 (pos, 1.0)).xyz;\n"
          "  gl_Position = MVP * vec4 (pos + origin, 1.0);\n"
          "}\n";

        return source;
      }

      std::string Renderer::fragment_shader_source() const
      {
        std::string source;
        source +=
          "uniform int use_lighting, hide_neg_lobes;\n"
          "uniform float ambient, diffuse, specular, shine;\n"
          "uniform vec3 light_pos;\n"
          "in vec3 position, color, normal;\n"
          "in float amplitude;\n"
          "out vec3 final_color;\n"
          "void main() {\n"
          "  if (amplitude < 0.0) {\n"
          "    if (hide_neg_lobes != 0) discard;\n"
          "    final_color = vec3(1.0,1.0,1.0);\n"
          "  }\n"
          "  else final_color = color;\n"
          "  if (use_lighting != 0) {\n"
          "    vec3 norm = normalize (normal);\n"
          "    if (amplitude < 0.0)\n"
          "      norm = -norm;\n"
          "    final_color *= ambient + diffuse * clamp (dot (norm, light_pos), 0, 1);\n"
          "    final_color += specular * pow (clamp (dot (reflect (-light_pos, norm), normalize(position)), 0, 1), shine);\n"
          "  }\n"
          "}\n";
        return source;
      }
















      Renderer::SH::~SH()
      {
        Renderer::GrabContext context (parent.context_);
        half_sphere.vertex_buffer.clear();
        half_sphere.index_buffer.clear();
        surface_buffer.clear();
        VAO.clear();
      }


      void Renderer::SH::initGL()
      {
        GL_CHECK_ERROR;
        Renderer::GrabContext context (parent.context_);
        half_sphere.vertex_buffer.gen();
        surface_buffer.gen();
        half_sphere.index_buffer.gen();
        VAO.gen();
        VAO.bind();

        half_sphere.vertex_buffer.bind (gl::ARRAY_BUFFER);
        gl::EnableVertexAttribArray (0);
        gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, sizeof(Shapes::HalfSphere::Vertex), (void*)0);

        surface_buffer.bind (gl::ARRAY_BUFFER);
        gl::EnableVertexAttribArray (1);
        gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 3*sizeof(GLfloat), (void*)0);

        half_sphere.index_buffer.bind();
        GL_CHECK_ERROR;
      }



      void Renderer::SH::update_mesh (const size_t LOD, const int lmax)
      {
        INFO ("updating ODF SH renderer transform...");
        QApplication::setOverrideCursor (Qt::BusyCursor);
        half_sphere.LOD (LOD);
        update_transform (half_sphere.vertices, lmax);
        QApplication::restoreOverrideCursor();
      }



      void Renderer::SH::update_transform (const std::vector<Shapes::HalfSphere::Vertex>& vertices, int lmax)
      {
        // order is r, del, daz

        transform = decltype(transform)::Zero (3*vertices.size(), Math::SH::NforL (lmax));

        for (size_t n = 0; n < vertices.size(); ++n) {
          for (int l = 0; l <= lmax; l+=2) {
            for (int m = 0; m <= l; m++) {
              const int idx (Math::SH::index (l,m));
              transform (3*n, idx) = transform(3*n, idx-2*m) = SH_NON_M0_SCALE_FACTOR Math::Legendre::Plm_sph<float> (l, m, vertices[n][2]);
            }
          }

          bool atpole (vertices[n][0] == 0.0 && vertices[n][1] == 0.0);
          float az = atpole ? 0.0 : atan2 (vertices[n][1], vertices[n][0]);

          for (int l = 2; l <= lmax; l+=2) {
            const int idx (Math::SH::index (l,0));
            transform (3*n+1, idx) = transform (3*n, idx+1) * sqrt (float (l* (l+1)));
          }

          for (int m = 1; m <= lmax; m++) {
            float caz = cos (m*az);
            float saz = sin (m*az);
            for (int l = 2* ( (m+1) /2); l <= lmax; l+=2) {
              const int idx (Math::SH::index (l,m));
              transform (3*n+1, idx) = - transform (3*n, idx-1) * sqrt (float ( (l+m) * (l-m+1)));
              if (l > m) 
                transform (3*n+1,idx) += transform (3*n, idx+1) * sqrt (float ( (l-m) * (l+m+1)));
              transform (3*n+1, idx) /= 2.0;

              const int idx2 (idx-2*m);
              if (atpole) {
                transform (3*n+2, idx) = - transform (3*n+1, idx) * saz;
                transform (3*n+2, idx2) = transform (3*n+1, idx) * caz;
              }
              else {
                float tmp (m * transform (3*n, idx));
                transform (3*n+2, idx) = -tmp * saz;
                transform (3*n+2, idx2) = tmp * caz;
              }

              transform (3*n+1, idx2) = transform (3*n+1, idx) * saz;
              transform (3*n+1, idx) *= caz;
            }
          }

          for (int m = 1; m <= lmax; m++) {
            float caz = cos (m*az);
            float saz = sin (m*az);
            for (int l = 2* ( (m+1) /2); l <= lmax; l+=2) {
              const int idx (Math::SH::index (l,m));
              transform (3*n, idx) *= caz;
              transform (3*n, idx-2*m) *= saz;
            }
          }

        }

      }








      Renderer::Dixel::~Dixel()
      {
        Renderer::GrabContext context (parent.context_);
        vertex_buffer.clear();
        value_buffer.clear();
        normal_buffer.clear();
        VAO.clear();
      }



      void Renderer::Dixel::initGL()
      {
        GL_CHECK_ERROR;
        Renderer::GrabContext context (parent.context_);
        vertex_buffer.gen();
        value_buffer.gen();
        normal_buffer.gen();
        VAO.gen();
        VAO.bind();

        vertex_buffer.bind (gl::ARRAY_BUFFER);
        gl::EnableVertexAttribArray (0);
        gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 3*sizeof(GLfloat), (void*)0);

        value_buffer.bind (gl::ARRAY_BUFFER);
        gl::EnableVertexAttribArray (1);
        gl::VertexAttribPointer (1, 1, gl::FLOAT, gl::FALSE_, sizeof(GLfloat), (void*)0);

        normal_buffer.bind (gl::ARRAY_BUFFER);
        gl::EnableVertexAttribArray (2);
        gl::VertexAttribPointer (2, 3, gl::FLOAT, gl::FALSE_, 3*sizeof(GLfloat), (void*)0);
        GL_CHECK_ERROR;
      }



      void Renderer::Dixel::update_mesh (const MR::DWI::Directions::Set& dirs)
      {
        INFO ("updating ODF dixel renderer transform...");
        QApplication::setOverrideCursor (Qt::BusyCursor);
        update_dixels (dirs);
        QApplication::restoreOverrideCursor();
      }



      void Renderer::Dixel::set_data (const vector_t& data, int buffer_ID) const
      {
        (void)buffer_ID;
        assert (size_t(data.size()) == directions.size());
        GL_CHECK_ERROR;

        std::vector<GLfloat> values;
        std::vector<Eigen::Vector3f> normals;
        for (auto i : polygons) {
          std::array<Eigen::Vector3f,3> v { data[i[0]] * directions[i[0]] * i.reverse(0),
                                            data[i[1]] * directions[i[1]] * i.reverse(1),
                                            data[i[2]] * directions[i[2]] * i.reverse(2) };
          const Eigen::Vector3f normal = ((v[1]-v[0]).cross (v[2]-v[1])).normalized();
          normals.push_back (normal);
          normals.push_back (normal);
          normals.push_back (normal);
          values.push_back (data[i[0]]);
          values.push_back (data[i[1]]);
          values.push_back (data[i[2]]);
        }

        Renderer::GrabContext context (parent.context_);
        VAO.bind();
        value_buffer.bind (gl::ARRAY_BUFFER);
        gl::BufferData (gl::ARRAY_BUFFER, values.size()*sizeof(GLfloat), &values[0], gl::STREAM_DRAW);
        gl::VertexAttribPointer (1, 1, gl::FLOAT, gl::FALSE_, sizeof(GLfloat), (void*)0);

        normal_buffer.bind (gl::ARRAY_BUFFER);
        gl::BufferData (gl::ARRAY_BUFFER, normals.size()*3*sizeof(float), normals[0].data(), gl::STREAM_DRAW);
        gl::VertexAttribPointer (2, 3, gl::FLOAT, gl::FALSE_, 3*sizeof(GLfloat), (void*)0);
        GL_CHECK_ERROR;
      }



      void Renderer::Dixel::update_dixels (const MR::DWI::Directions::Set& dirs)
      {
        directions.clear();
        polygons.clear();

        for (size_t i = 0; i != dirs.size(); ++i) {
          directions.push_back (dirs[i]);
          for (auto j : dirs.get_adj_dirs(i)) {
            if (j > i) {
              for (auto k : dirs.get_adj_dirs(j)) {
                if (k > j) {

                  // k's adjacent direction list MUST contain i!
                  for (auto I : dirs.get_adj_dirs (k)) {
                    if (I == i) {

                      const Eigen::Vector3f mean_dir ((dirs[i] + dirs[j] + dirs[k]).normalized());
                      size_t reversed = 3;
                      if (dirs[i].dot (mean_dir) < 0.0f)
                        reversed = 0;
                      else if (dirs[j].dot (mean_dir) < 0.0f)
                        reversed = 1;
                      else if (dirs[k].dot (mean_dir) < 0.0f)
                        reversed = 2;
                      // Conform to right hand rule
                      const Eigen::Vector3f normal (((dirs[j]-dirs[i]).cross (dirs[k]-dirs[j])).normalized());
                      if (normal.dot (mean_dir) < 0.0f) {
                        if (reversed == 1)
                          reversed = 2;
                        else if (reversed == 2)
                          reversed = 1;
                        polygons.push_back (Polygon (i, k, j, reversed));
                      } else {
                        polygons.push_back (Polygon (i, j, k, reversed));
                      }

                    }
                  }

                }
              }
            }
          }
        }

        std::vector<Eigen::Vector3f> vertices (3 * polygons.size());
        for (size_t i = 0; i != polygons.size(); ++i) {
          vertices[3*i]   = directions[polygons[i][0]] * polygons[i].reverse (0);
          vertices[3*i+1] = directions[polygons[i][1]] * polygons[i].reverse (1);
          vertices[3*i+2] = directions[polygons[i][2]] * polygons[i].reverse (2);
        }

        GL_CHECK_ERROR;
        Renderer::GrabContext context (parent.context_);
        VAO.bind();
        vertex_buffer.bind (gl::ARRAY_BUFFER);
        gl::BufferData (gl::ARRAY_BUFFER, vertices.size()*sizeof(Eigen::Vector3f), &vertices[0][0], gl::STATIC_DRAW);
        gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 3*sizeof(GLfloat), (void*)0);
        GL_CHECK_ERROR;
      }



    }
  }
}




