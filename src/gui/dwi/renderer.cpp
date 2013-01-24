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
#include <QApplication>

#include "math/legendre.h"
#include "gui/dwi/renderer.h"

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

  static uint32_t initial_indices[NUM_INDICES][3] = {
    {0,1,2}, {0,2,5}, {2,1,4}, {4,1,6},
    {8,6,3}, {8,3,7}, {7,3,0}, {0,3,1},
    {3,6,1}, {5,7,0}
  };

}

namespace
{
  const char* vertex_shader_source =
    "uniform int color_by_direction, use_normals, reverse;\n"
    "uniform float scale;\n"
    "varying vec4 color, ambient;\n"
    "varying vec3 normal, lightDir, halfVector;\n"
    "varying float amplitude;\n"
    "void main () {\n"
    "  vec4 vertex = gl_Vertex;\n"
    "  normal = gl_Normal;\n"
    "  amplitude = normal.x;\n"
    "  if (use_normals != 0) {\n"
    "    bool atpole = ( vertex.x == 0.0 && vertex.y == 0.0 );\n"
    "    float az = atpole ? 0.0 : atan (vertex.y, vertex.x);\n"
    "    float caz = cos (az), saz = sin (az), cel = vertex.z, sel = sqrt (1.0 - cel*cel);\n"
    "    vec3 d1;\n"
    "    if (atpole)\n"
    "      d1 = vec3 (-normal.x*saz, normal.x*caz, normal.z);\n"
    "    else\n"
    "      d1 = vec3 (normal.z*caz*sel - normal.x*sel*saz, normal.z*saz*sel + normal.x*sel*caz, normal.z*cel);\n"
    "    vec3 d2 = vec3 (-normal.y*caz*sel - normal.x*caz*cel, -normal.y*saz*sel - normal.x*saz*cel, -normal.y*cel + normal.x*sel);\n"
    "    normal = cross (d1, d2);\n"
    "    if (reverse != 0)\n"
    "      normal = -normal;\n"
    "    normal = normalize (gl_NormalMatrix * normal);\n"
    "    lightDir = normalize (vec3 (gl_LightSource[0].position));\n"
    "    halfVector = normalize (gl_LightSource[0].halfVector.xyz);\n"
    "    ambient = gl_LightSource[0].ambient + gl_LightModel.ambient;\n"
    "  }\n"
    "  if (color_by_direction != 0) { color.rgb = abs (vertex.xyz); color.a = 1.0; }\n"
    "  else { color = gl_Color; }\n"
    "  vertex.xyz *= amplitude * scale;\n"
    "  if (reverse != 0)\n"
    "    vertex.xyz = -vertex.xyz;\n"
    "  gl_Position = gl_ModelViewProjectionMatrix * vertex;\n"
    "}\n";


  const char* fragment_shader_source =
    "uniform int use_normals, hide_neg_lobes;\n"
    "varying vec4 color, diffuse, ambient;\n"
    "varying vec3 normal, lightDir, halfVector;\n"
    "varying float amplitude;\n"
    "void main() {\n"
    "  vec4 frag_color, actual_color;\n"
    "  vec3 n, halfV;\n"
    "  float NdotL, NdotHV;\n"
    "  if (amplitude < 0.0) {\n"
    "    if (hide_neg_lobes != 0) discard;\n"
    "    actual_color = vec4(1.0,1.0,1.0,1.0);\n"
    "  }\n"
    "  else actual_color = color;\n"
    "  n = normalize (normal);\n"
    "  if (use_normals != 0) {\n"
    "    if (amplitude < 0.0) n = -n;\n"
    "    NdotL = dot (n,lightDir);\n"
    "    frag_color = actual_color * ambient;\n"
    "    if (NdotL > 0.0) {\n"
    "      frag_color += gl_LightSource[0].diffuse * NdotL * actual_color;\n"
    "      halfV = normalize(halfVector);\n"
    "      NdotHV = max(dot(n,halfV),0.0);\n"
    "      frag_color += gl_FrontMaterial.specular * gl_LightSource[0].specular * pow (NdotHV, gl_FrontMaterial.shininess);\n"
    "    }\n"
    "  }\n"
    "  else frag_color = actual_color;\n"
    "  gl_FragColor = frag_color;\n"
    "}\n";

}


namespace MR
{
  namespace GUI
  {
    namespace DWI
    {

      void Renderer::init ()
      {
        GL::Shader::Vertex vertex_shader (vertex_shader_source);
        GL::Shader::Fragment fragment_shader (fragment_shader_source);
        shader_program.attach (vertex_shader);
        shader_program.attach (fragment_shader);
        shader_program.link();
      }








      void Renderer::draw (float scale, bool use_normals, const float* colour)
      {
        if (recompute_mesh) 
          compute_mesh();

        if (recompute_amplitudes) 
          compute_amplitudes();

        glPushClientAttrib (GL_CLIENT_VERTEX_ARRAY_BIT);
        glEnableClientState (GL_VERTEX_ARRAY);
        glVertexPointer (3, GL_FLOAT, sizeof (Vertex), &vertices[0]);

        glEnableClientState (GL_NORMAL_ARRAY);
        glNormalPointer (GL_FLOAT, 0, &amplitudes_and_derivatives[0]);

        if (colour) 
          glColor3fv (colour);

        shader_program.start();
        glUniform1f (glGetUniformLocation (shader_program, "scale"), scale);
        glUniform1i (glGetUniformLocation (shader_program, "color_by_direction"), colour ? 0 : 1);
        glUniform1i (glGetUniformLocation (shader_program, "use_normals"), use_normals ? 1 : 0);
        glUniform1i (glGetUniformLocation (shader_program, "hide_neg_lobes"), hide_neg_lobes ? 1 : 0);
        GLuint reverse = glGetUniformLocation (shader_program, "reverse");

        glUniform1i (reverse, 0);
        glDrawElements (GL_TRIANGLES, 3*indices.size(), GL_UNSIGNED_INT, &indices[0]);
        glUniform1i (reverse, 1);
        glDrawElements (GL_TRIANGLES, 3*indices.size(), GL_UNSIGNED_INT, &indices[0]);

        shader_program.stop();
        glPopClientAttrib();

      }





      void Renderer::compute_mesh ()
      {
        recompute_mesh = false;
        recompute_amplitudes = true;
        INFO ("updating SH renderer transform...");
        QApplication::setOverrideCursor (Qt::BusyCursor);

        indices.clear();
        vertices.clear();

        for (int n = 0; n < NUM_VERTICES; n++)
          vertices.push_back (initial_vertices[n]);

        for (int n = 0; n < NUM_INDICES; n++) 
          indices.push_back (initial_indices[n]);

        std::map<Edge,uint32_t> edges;

        for (int lod = 0; lod < lod_computed; lod++) {
          uint32_t num = indices.size();
          for (GLuint n = 0; n < num; n++) {
            uint32_t index1, index2, index3;

            Edge E (indices[n][0], indices[n][1]);
            std::map<Edge,uint32_t>::const_iterator iter;
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

        compute_transform ();

        QApplication::restoreOverrideCursor();
      }





      void Renderer::compute_amplitudes ()
      {
        recompute_amplitudes = false;
        INFO ("updating values...");

        int actual_lmax = Math::SH::LforN (SH.size());
        if (actual_lmax > lmax_computed) actual_lmax = lmax_computed;
        size_t nSH = Math::SH::NforL (actual_lmax);

        amplitudes_and_derivatives.resize (transform.rows());

        Math::Matrix<float> M (transform.sub (0, transform.rows(), 0, nSH));
        Math::Vector<float> S (SH.sub (0, nSH));
        Math::Vector<float> A (&amplitudes_and_derivatives[0], transform.rows());

        Math::mult (A, M, S);
      }





      void Renderer::compute_transform ()
      {
        // order is r, del, daz

        transform.allocate (3*vertices.size(), Math::SH::NforL (lmax_computed));
        transform.zero();

        for (size_t n = 0; n < vertices.size(); ++n) {
/*
        GLfloat* r (get_r (row));
        GLfloat* daz (get_daz (row));
        GLfloat* del (get_del (row));
        memset (r, 0, 3*nsh*sizeof (GLfloat));
*/
          for (int l = 0; l <= lmax_computed; l+=2) {
            for (int m = 0; m <= l; m++) {
              const int idx (Math::SH::index (l,m));
              transform (3*n, idx) = transform(3*n, idx-2*m) = Math::Legendre::Plm_sph<float> (l, m, vertices[n][2]);
            }
          }

          bool atpole (vertices[n][0] == 0.0 && vertices[n][1] == 0.0);
          float az = atpole ? 0.0 : atan2 (vertices[n][1], vertices[n][0]);

          for (int l = 2; l <= lmax_computed; l+=2) {
            const int idx (Math::SH::index (l,0));
            //del[idx] = r[idx+1] * sqrt (float (l* (l+1)));
            transform (3*n+1, idx) = transform (3*n, idx+1) * sqrt (float (l* (l+1)));
          }

          for (int m = 1; m <= lmax_computed; m++) {
            float caz = cos (m*az);
            float saz = sin (m*az);
            for (int l = 2* ( (m+1) /2); l <= lmax_computed; l+=2) {
              const int idx (Math::SH::index (l,m));
              //del[idx] = - r[idx-1] * sqrt (float ( (l+m) * (l-m+1)));
              transform (3*n+1, idx) = - transform (3*n, idx-1) * sqrt (float ( (l+m) * (l-m+1)));
              if (l > m) 
                transform (3*n+1,idx) += transform (3*n, idx+1) * sqrt (float ( (l-m) * (l+m+1)));
              //del[idx] += r[idx+1] * sqrt (float ( (l-m) * (l+m+1)));
              //del[idx] /= 2.0;
              transform (3*n+1, idx) /= 2.0;

              const int idx2 (idx-2*m);
              if (atpole) {
                //daz[idx] = - del[idx] * saz;
                //daz[idx2] = del[idx] * caz;
                transform (3*n+2, idx) = - transform (3*n+1, idx) * saz;
                transform (3*n+2, idx2) = transform (3*n+1, idx) * caz;
              }
              else {
                //float tmp (m * r[idx]);
                float tmp (m * transform (3*n, idx));
                //daz[idx] = - tmp * saz;
                //daz[idx2] = tmp * caz;
                transform (3*n+2, idx) = -tmp * saz;
                transform (3*n+2, idx2) = tmp * caz;
              }

              //del[idx2] = del[idx] * saz;
              transform (3*n+1, idx2) = transform (3*n+1, idx) * saz;
              //del[idx] *= caz;
              transform (3*n+1, idx) *= caz;
            }
          }

          for (int m = 1; m <= lmax_computed; m++) {
            float caz = cos (m*az);
            float saz = sin (m*az);
            for (int l = 2* ( (m+1) /2); l <= lmax_computed; l+=2) {
              const int idx (Math::SH::index (l,m));
              //r[idx] *= caz;
              transform (3*n, idx) *= caz;
              //r[idx-2*m] *= saz;
              transform (3*n, idx-2*m) *= saz;
            }
          }

        }

      }



    }
  }
}




