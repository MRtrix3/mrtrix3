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

#define X .525731112119133606
#define Z .850650808352039932

#define NUM_VERTICES 9
#define NUM_INDICES 10

namespace
{

  static float initial_vertices[NUM_VERTICES][3] = {
    {-X, 0.0, Z}, {X, 0.0, Z}, {0.0, Z, X}, {0.0, -Z, X},
    {Z, X, 0.0}, {-Z, X, 0.0}, {Z, -X, 0.0}, {-Z, -X, 0.0},
    {0.0, -Z, -X}
  };

  static GLuint initial_indices[NUM_INDICES][3] = {
    {0,1,2}, {0,2,5}, {2,1,4}, {4,1,6},
    {8,6,3}, {8,3,7}, {7,3,0}, {0,3,1},
    {3,6,1}, {5,7,0}
  };

}

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


      namespace {

        class Triangle
        {
          public:
            Triangle () { }
            Triangle (const GLuint x[3]) {
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
            GLuint  index[3];
        };

        class Edge
        {
          public:
            Edge (const Edge& E) {
              set (E.i1, E.i2);
            }
            Edge (GLuint a, GLuint b) {
              set (a,b);
            }
            bool operator< (const Edge& E) const {
              return (i1 < E.i1 ? true : i2 < E.i2);
            }
            void set (GLuint a, GLuint b) {
              if (a < b) {
                i1 = a;
                i2 = b;
              }
              else {
                i1 = b;
                i2 = a;
              }
            }
            GLuint i1;
            GLuint i2;
        };


      }




      void Renderer::initGL ()
      {
        GL::Shader::Vertex vertex_shader (vertex_shader_source);
        GL::Shader::Fragment fragment_shader (fragment_shader_source);
        shader_program.attach (vertex_shader);
        shader_program.attach (fragment_shader);
        shader_program.link();

        vertex_buffer.gen();
        surface_buffer.gen();
        index_buffer.gen();
        vertex_array_object.gen();
        vertex_array_object.bind();

        vertex_buffer.bind (gl::ARRAY_BUFFER);
        gl::EnableVertexAttribArray (0);
        gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, sizeof(Vertex), (void*)0);

        surface_buffer.bind (gl::ARRAY_BUFFER);
        gl::EnableVertexAttribArray (1);
        gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 3*sizeof(GLfloat), (void*)0);

        index_buffer.bind (gl::ELEMENT_ARRAY_BUFFER);
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







      void Renderer::update_mesh (int LOD, int lmax)
      {
        INFO ("updating SH renderer transform...");
        QApplication::setOverrideCursor (Qt::BusyCursor);

        std::vector<Vertex> vertices;
        std::vector<Triangle> indices;

        for (int n = 0; n < NUM_VERTICES; n++)
          vertices.push_back (initial_vertices[n]);

        for (int n = 0; n < NUM_INDICES; n++) 
          indices.push_back (initial_indices[n]);

        std::map<Edge,GLuint> edges;

        for (int lod = 0; lod < LOD; lod++) {
          GLuint num = indices.size();
          for (GLuint n = 0; n < num; n++) {
            GLuint index1, index2, index3;

            Edge E (indices[n][0], indices[n][1]);
            std::map<Edge,GLuint>::const_iterator iter;
            if ( (iter = edges.find (E)) == edges.end()) {
              index1 = vertices.size();
              edges[E] = index1;
              vertices.push_back (Vertex (vertices, indices[n][0], indices[n][1]));
            }
            else index1 = iter->second;

            E.set (indices[n][1], indices[n][2]);
            if ( (iter = edges.find (E)) == edges.end()) {
              index2 = vertices.size();
              edges[E] = index2;
              vertices.push_back (Vertex (vertices, indices[n][1], indices[n][2]));
            }
            else index2 = iter->second;

            E.set (indices[n][2], indices[n][0]);
            if ( (iter = edges.find (E)) == edges.end()) {
              index3 = vertices.size();
              edges[E] = index3;
              vertices.push_back (Vertex (vertices, indices[n][2], indices[n][0]));
            }
            else index3 = iter->second;

            indices.push_back (Triangle (indices[n][0], index1, index3));
            indices.push_back (Triangle (indices[n][1], index2, index1));
            indices.push_back (Triangle (indices[n][2], index3, index2));
            indices[n].set (index1, index2, index3);
          }
        }

        update_transform (vertices, lmax);

        vertex_buffer.bind (gl::ARRAY_BUFFER);
        gl::BufferData (gl::ARRAY_BUFFER, vertices.size()*sizeof(Vertex), &vertices[0][0], gl::STATIC_DRAW);

        num_indices = 3*indices.size();
        index_buffer.bind (gl::ELEMENT_ARRAY_BUFFER);
        gl::BufferData (gl::ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(Triangle), &indices[0], gl::STATIC_DRAW);

        QApplication::restoreOverrideCursor();
      }





#ifdef USE_NON_ORTHONORMAL_SH_BASIS
# define SH_NON_M0_SCALE_FACTOR
#else
# define SH_NON_M0_SCALE_FACTOR (m?M_SQRT2:1.0)*
#endif



      void Renderer::update_transform (const std::vector<Vertex>& vertices, int lmax)
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




