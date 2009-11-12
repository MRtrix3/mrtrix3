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

#include <map>
#include <QApplication>

#include "dwi/renderer.h"
#include "math/legendre.h"

#define X .525731112119133606 
#define Z .850650808352039932

#define NUM_VERTICES 9
#define NUM_INDICES 10

namespace {

  static GLfloat initial_vertices[NUM_VERTICES][3] = {    
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

namespace {
  const char* vertex_shader_source = 
    "uniform int color_by_direction, use_normals, reverse;"
    "varying vec4 color, ambient;"
    "varying vec3 normal, lightDir, halfVector;"
    "varying float direction;"
    "void main () {"
    "  vec4 vertex = gl_Vertex;"
    "  normal = gl_Normal;"
    "  direction = dot (vertex.xyz, normal);"
    "  if (reverse != 0) {"
    "     vertex.xyz = -vertex.xyz;"
    "     normal = -normal;"
    "  }"
    "  if (use_normals != 0) {"
    "    normal = normalize (gl_NormalMatrix * normal);"
    "    lightDir = normalize (vec3 (gl_LightSource[0].position));"
    "    halfVector = normalize (gl_LightSource[0].halfVector.xyz);"
    "    ambient = gl_LightSource[0].ambient + gl_LightModel.ambient;"
    "  }"
    "  if (color_by_direction != 0) { color.rgb = abs (normalize (vertex.xyz)); color.a = 1.0; }"
    "  else { color = gl_Color; }"
    "  gl_Position = gl_ModelViewProjectionMatrix * vertex;"
    "}";

  const char* fragment_shader_source = 
    "uniform int use_normals, hide_neg_lobes;"
    "varying vec4 color, diffuse, ambient;"
    "varying vec3 normal, lightDir, halfVector;"
    "varying float direction;"
    "void main() {"
    "  vec4 frag_color, actual_color;"
    "  vec3 n, halfV;"
    "  float NdotL, NdotHV;"
    "  if (direction < 0.0) {"
    "    if (hide_neg_lobes != 0) discard;"
    "    actual_color = vec4(1.0,1.0,1.0,1.0);"
    "  }"
    "  else actual_color = color;"
    "  n = normalize (normal);"
    "  if (use_normals != 0) {"
    "    if (direction < 0.0) n = -n;"
    "    NdotL = dot (n,lightDir);"
    "    frag_color = actual_color * ambient;"
    "    if (NdotL > 0.0) {"
    "      frag_color += gl_LightSource[0].diffuse * NdotL * actual_color;"
    "      halfV = normalize(halfVector);"
    "      NdotHV = max(dot(n,halfV),0.0);"
    "      frag_color += gl_FrontMaterial.specular * gl_LightSource[0].specular * pow (NdotHV, gl_FrontMaterial.shininess);"
    "    }"
    "  }"
    "  else frag_color = actual_color;"
    "  gl_FragColor = frag_color;"
    "}";

}


namespace MR {
  namespace DWI {

    void Renderer::init () 
    { 
      GL::Shader::check();

      vertex_shader = glCreateShaderObjectARB (GL_VERTEX_SHADER_ARB);
      fragment_shader = glCreateShaderObjectARB (GL_FRAGMENT_SHADER_ARB);
      glShaderSourceARB (vertex_shader, 1, &vertex_shader_source, NULL);
      glShaderSourceARB (fragment_shader, 1, &fragment_shader_source, NULL);
      glCompileShaderARB (vertex_shader);
      glCompileShaderARB (fragment_shader);
      shader_program = glCreateProgramObjectARB();
      GL::Shader::print_log ("orientation plot vertex shader", vertex_shader);
      GL::Shader::print_log ("orientation plot fragment shader", fragment_shader);

      glAttachObjectARB (shader_program, vertex_shader);
      glAttachObjectARB (shader_program, fragment_shader);
      glLinkProgramARB (shader_program);
      GL::Shader::print_log ("orientation plot shader program", shader_program);
    }




    Renderer::~Renderer () 
    { 
      glDetachObjectARB (shader_program, vertex_shader);
      glDetachObjectARB (shader_program, fragment_shader);
      glDeleteObjectARB (vertex_shader);
      glDeleteObjectARB (fragment_shader);
      glDeleteObjectARB (shader_program); 
      clear();
    }





    void Renderer::draw (bool use_normals, bool hide_neg_lobes, const float* colour) const
    { 
      glUseProgramObjectARB (shader_program);
      glUniform1i (glGetUniformLocation (shader_program, "color_by_direction"), colour ? 0 : 1);
      glUniform1i (glGetUniformLocation (shader_program, "use_normals"), use_normals ? 1 : 0);
      glUniform1i (glGetUniformLocation (shader_program, "hide_neg_lobes"), hide_neg_lobes ? 1 : 0);
      glUniform1i (glGetUniformLocation (shader_program, "reverse"), 0);

      if (colour) glColor3fv (colour);

      glPushClientAttrib (GL_CLIENT_VERTEX_ARRAY_BIT);
      glEnableClientState (GL_VERTEX_ARRAY);
      glEnableClientState (GL_NORMAL_ARRAY);
      glVertexPointer (3, GL_FLOAT, sizeof(Vertex), &vertices[0].P);
      glNormalPointer (GL_FLOAT, sizeof(Vertex), &vertices[0].N);

      glDrawElements (GL_TRIANGLES, 3*indices.size(), GL_UNSIGNED_INT, &indices[0]);
      glUniform1i (glGetUniformLocation (shader_program, "reverse"), 1);
      glDrawElements (GL_TRIANGLES, 3*indices.size(), GL_UNSIGNED_INT, &indices[0]);

      glPopClientAttrib();
      glUseProgramObjectARB (0);
    }





    void Renderer::precompute (int lmax, int lod)
    {
      if (lmax <= lmax_computed && lod == lod_computed) return;

      info ("updating SH renderer transform...");
      QApplication::setOverrideCursor (Qt::BusyCursor);

      if (lmax > lmax_computed) {
        nsh = Math::SH::NforL (lmax); 
        row_size = 3 + 3*nsh; 
        lmax_computed = lmax;
      }

      clear();

      for (int n = 0; n < NUM_VERTICES; n++) 
        push_back (initial_vertices[n]);

      indices.resize (NUM_INDICES);
      for (int n = 0; n < NUM_INDICES; n++) {
        indices[n][0] = initial_indices[n][0];
        indices[n][1] = initial_indices[n][1];
        indices[n][2] = initial_indices[n][2];
      }

      std::map<Edge,size_t> edges;

      for (lod_computed = 0; lod_computed < lod; lod_computed++) {
        size_t num = indices.size();
        for (GLuint n = 0; n < num; n++) {
          size_t index1, index2, index3;

          Edge E (indices[n][0], indices[n][1]);
          std::map<Edge,size_t>::const_iterator iter;
          if ((iter = edges.find (E)) == edges.end()) {
            index1 = rows.size();
            edges[E] = index1;
            push_back (indices[n][0], indices[n][1]);
          }
          else index1 = iter->second;

          E.set (indices[n][1], indices[n][2]);
          if ((iter = edges.find (E)) == edges.end()) {
            index2 = rows.size();
            edges[E] = index2;
            push_back (indices[n][1], indices[n][2]);
          }
          else index2 = iter->second;

          E.set (indices[n][2], indices[n][0]);
          if ((iter = edges.find (E)) == edges.end()) {
            index3 = rows.size();
            edges[E] = index3;
            push_back (indices[n][2], indices[n][0]);
          }
          else index3 = iter->second;

          indices.push_back (Triangle (indices[n][0], index1, index3));
          indices.push_back (Triangle (indices[n][1], index2, index1));
          indices.push_back (Triangle (indices[n][2], index3, index2));
          indices[n].set (index1, index2, index3);
        }
      }

      vertices.resize (rows.size());
      QApplication::restoreOverrideCursor();
    }






    void Renderer::calculate (const std::vector<float>& values, int lmax)
    {
      info ("updating values...");

      int actual_lmax = Math::SH::LforN (values.size());
      if (lmax > lmax_computed) lmax = lmax_computed;
      if (actual_lmax > lmax) actual_lmax = lmax;
      size_t nsh = Math::SH::NforL (actual_lmax);

      for (size_t n = 0; n < vertices.size(); n++) {
        Vertex& V (vertices[n]);
        GLfloat* row (rows[n]);
        GLfloat* row_r (get_r (row));
        GLfloat* row_daz (get_daz (row));
        GLfloat* row_del (get_del (row));

        float r (0.0), daz (0.0), del (0.0);

        for (size_t i = 0; i < nsh; i++) {
          r += row_r[i] * values[i]; 
          daz += row_daz[i] * values[i]; 
          del += row_del[i] * values[i]; 
        }

        V.P[0] = r*row[0];
        V.P[1] = r*row[1];
        V.P[2] = r*row[2];

        bool atpole (row[0] == 0.0 && row[1] == 0.0);
        float az = atpole ? 0.0 : atan2 (row[1], row[0]);

        float caz = cos (az);
        float saz = sin (az);
        float cel = row[2];
        float sel = sqrt (1.0 - Math::pow2 (cel));

        V.P[0] = r*caz*sel;
        V.P[1] = r*saz*sel;
        V.P[2] = r*cel;

        float d1[3], d2[3];

        if (atpole) {
          d1[0] =  -r*saz;
          d1[1] =  r*caz;
          d1[2] =  daz;
        }
        else {
          d1[0] = daz*caz*sel-r*sel*saz;
          d1[1] = daz*saz*sel+r*sel*caz;
          d1[2] = daz*cel;
        }

        d2[0] = -del*caz*sel-r*caz*cel;
        d2[1] = -del*saz*sel-r*saz*cel;
        d2[2] = -del*cel+r*sel;

        Math::cross (V.N, d1, d2);
      }

    }





    void Renderer::precompute_row (GLfloat* row) 
    {
      rows.push_back (row);
      Math::normalise (row);

      GLfloat* r (get_r (row));
      GLfloat* daz (get_daz (row));
      GLfloat* del (get_del (row));
      memset (r, 0, 3*nsh*sizeof(GLfloat));

      for (int l = 0; l <= lmax_computed; l+=2) {
        for (int m = 0; m <= l; m++) {
          const int idx (Math::SH::index(l,m));
          r[idx] = Math::Legendre::Plm_sph<float> (l, m, row[2]);
          if (m) r[idx-2*m] = r[idx];
        }
      }

      bool atpole (row[0] == 0.0 && row[1] == 0.0);
      float az = atpole ? 0.0 : atan2 (row[1], row[0]);

      for (int l = 2; l <= lmax_computed; l+=2) {
        const int idx (Math::SH::index(l,0));
        del[idx] = r[idx+1] * sqrt (float(l*(l+1)));
      }

      for (int m = 1; m <= lmax_computed; m++) {
        float caz = cos (m*az); 
        float saz = sin (m*az); 
        for (int l = 2*((m+1)/2); l <= lmax_computed; l+=2) {
          const int idx (Math::SH::index(l,m));
          del[idx] = - r[idx-1] * sqrt (float ((l+m)*(l-m+1)));
          if (l > m) del[idx] += r[idx+1] * sqrt (float ((l-m)*(l+m+1)));
          del[idx] /= 2.0;

          const int idx2 (idx-2*m);
          if (atpole) {
            daz[idx] = - del[idx] * saz;
            daz[idx2] = del[idx] * caz;
          }
          else {
            GLfloat tmp (m * r[idx]);
            daz[idx] = - tmp * saz;
            daz[idx2] = tmp * caz;
          }

          del[idx2] = del[idx] * saz;
          del[idx] *= caz;
        }
      }

      for (int m = 1; m <= lmax_computed; m++) {
        float caz = cos (m*az); 
        float saz = sin (m*az); 
        for (int l = 2*((m+1)/2); l <= lmax_computed; l+=2) {
          const int idx (Math::SH::index(l,m));
          r[idx] *= caz;
          r[idx-2*m] *= saz;
        }
      }
      
    }



  }
}




