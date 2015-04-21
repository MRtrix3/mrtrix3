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

#include "gui/dwi/renderer.h"
#include "math/legendre.h"
#include "gui/projection.h"
#include "gui/opengl/lighting.h"


namespace
{
  const char* vertex_shader_source =
    "layout(location = 0) in vec3 vertex;\n"
    "layout(location = 1) in vec3 r_del_daz;\n"
    "uniform int color_by_direction, use_lighting, reverse, orthographic;\n"
    "uniform float scale;\n"
    "uniform vec3 constant_color, origin;\n"
    "uniform mat4 MV, MVP;\n"
    "out vec3 position, color, normal;\n"
    "out float amplitude;\n"
    "void main () {\n"
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
    "    normal = cross (d1, d2);\n"
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


  const char* fragment_shader_source =
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

}


namespace MR
{
  namespace GUI
  {
    namespace DWI
    {




      void Renderer::initGL ()
      {
        GL::Shader::Vertex vertex_shader (vertex_shader_source);
        GL::Shader::Fragment fragment_shader (fragment_shader_source);
        shader_program.attach (vertex_shader);
        shader_program.attach (fragment_shader);
        shader_program.link();

        sphere.vertex_buffer.gen();
        surface_buffer.gen();
        sphere.index_buffer.gen();
        vertex_array_object.gen();
        vertex_array_object.bind();

        sphere.vertex_buffer.bind (gl::ARRAY_BUFFER);
        gl::EnableVertexAttribArray (0);
        gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, sizeof(GUI::Sphere::Vertex), (void*)0);

        surface_buffer.bind (gl::ARRAY_BUFFER);
        gl::EnableVertexAttribArray (1);
        gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 3*sizeof(GLfloat), (void*)0);

        sphere.index_buffer.bind();
      }





      void Renderer::start (const Projection& projection, const GL::Lighting& lighting, float scale, 
          bool use_lighting, bool color_by_direction, bool hide_neg_lobes, bool orthographic) const
      {
        vertex_array_object.bind();
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







      void Renderer::update_mesh (const size_t LOD, const int lmax)
      {
        INFO ("updating SH renderer transform...");
        QApplication::setOverrideCursor (Qt::BusyCursor);
        sphere.LOD (LOD);
        update_transform (sphere.vertices, lmax);
        QApplication::restoreOverrideCursor();
      }







      void Renderer::update_transform (const std::vector<GUI::Sphere::Vertex>& vertices, int lmax)
      {
        // order is r, del, daz

        transform.allocate (3*vertices.size(), Math::SH::NforL (lmax));
        transform.zero();

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



    }
  }
}




