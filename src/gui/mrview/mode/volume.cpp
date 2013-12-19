/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#include "gui/opengl/lighting.h"
#include "math/vector.h"
#include "gui/mrview/mode/volume.h"
#include "gui/mrview/tool/base.h"
#include "gui/mrview/adjust_button.h"
#include "gui/dialog/lighting.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        std::string Volume::Shader::vertex_shader_source (const Displayable& object) 
        {
          return
            "layout(location=0) in vec3 vertpos;\n"
            "uniform mat4 M;\n"
            "out vec3 texcoord;\n"
            "void main () {\n"
            "  texcoord = vertpos;\n"
            "  gl_Position =  M * vec4 (vertpos,1);\n"
            "}\n";
        }

        std::string Volume::Shader::fragment_shader_source (const Displayable& object)
        {
          std::string source = 
            "uniform sampler3D image_sampler;\n"
            "in vec3 texcoord;\n"
            "uniform float offset, scale, alpha_scale, alpha_offset, alpha";
          if (object.use_discard_lower()) source += ", lower";
          if (object.use_discard_upper()) source += ", upper";
          source += ";\n";
          if (clip1) source += "vec4 clip1;\n";
          if (clip2) source += "vec4 clip2;\n";
          if (clip3) source += "vec4 clip3;\n";
          if (use_depth_testing) 
            source += 
              "uniform sampler2D depth_sampler;\n"
              "uniform mat4 M;\n"
              "uniform float ray_z;\n";


          source += 
            "uniform vec3 ray;\n"
            "out vec4 final_color;\n"
            "void main () {\n";


          if (clip1) source += "  clip1 = vec4 (1.0, 0.0, 0.0, 0.5);\n";
          if (clip2) source += "  clip2 = vec4 (0.0, 1.0, 0.0, 0.5);\n";
          if (clip3) source += "  clip3 = vec4 (0.0, 0.0, 1.0, 0.5);\n";

          source += 
            "  final_color = vec4 (0.0);\n"
            "  float dither = fract(sin(gl_FragCoord.x * 12.9898 + gl_FragCoord.y * 78.233) * 43758.5453);\n"
            "  vec3 coord = texcoord + ray * dither;\n";

          if (use_depth_testing) {
            source += 
              "  float depth = texelFetch (depth_sampler, ivec2(gl_FragCoord.xy), 0).r;\n"
              "  float current_depth = gl_FragCoord.z + ray_z * dither;\n";
          }

          source += 
            "  int nmax = 10000;\n"
            "  if (ray.x < 0.0) nmax = int (-texcoord.s/ray.x);\n"
            "  else if (ray.x > 0.0) nmax = int ((1.0-texcoord.s) / ray.x);\n"
            "  if (ray.y < 0.0) nmax = min (nmax, int (-texcoord.t/ray.y));\n"
            "  else if (ray.y > 0.0) nmax = min (nmax, int ((1.0-texcoord.t) / ray.y));\n"
            "  if (ray.z < 0.0) nmax = min (nmax, int (-texcoord.p/ray.z));\n"
            "  else if (ray.z > 0.0) nmax = min (nmax, int ((1.0-texcoord.p) / ray.z));\n";

          if (use_depth_testing)
            source += "  nmax = min (nmax, int ((depth - current_depth) / ray_z));\n";

          source +=
            "  if (nmax <= 0) return;\n"
            "  for (int n = 0; n < nmax; ++n) {\n"
            "    coord += ray;\n";

          if (clip1) source += "    if (dot (coord, clip1.xyz) > clip1.w)\n";
          if (clip2) source += "      if (dot (coord, clip2.xyz) > clip2.w)\n";
          if (clip3) source += "        if (dot (coord, clip3.xyz) > clip3.w)\n";
          if (clip1 || clip2 || clip3) source += "          continue;\n";

          source += 
            "    vec4 color = texture (image_sampler, coord);\n"
            "    float amplitude = " + std::string (ColourMap::maps[object.colourmap].amplitude) + ";\n"
            "    if (isnan(amplitude) || isinf(amplitude)) continue;\n";

          if (object.use_discard_lower())
            source += 
              "    if (amplitude < lower) continue;\n";

          if (object.use_discard_upper())
            source += 
              "    if (amplitude > upper) continue;\n";

          source +=
            "    if (amplitude < alpha_offset) continue;\n"
            "    color.a = clamp ((amplitude - alpha_offset) * alpha_scale, 0, alpha);\n";

          if (!ColourMap::maps[object.colourmap].special) {
            source += 
              "    amplitude = clamp (";
            if (object.scale_inverted()) 
              source += "1.0 -";
            source += 
              " scale * (amplitude - offset), 0.0, 1.0);\n";
          }

          source += 
            std::string ("    ") + ColourMap::maps[object.colourmap].mapping +
            "    final_color.rgb += (1.0 - final_color.a) * color.rgb * color.a;\n"
            "    final_color.a += color.a;\n" 
            "    if (final_color.a > 0.95) break;\n"
            "  }\n"
            "}\n";

          return source;
        }




        bool Volume::Shader::need_update (const Displayable& object) const 
        {
          if (mode.use_depth_testing != use_depth_testing)
            return true;
          return Displayable::Shader::need_update (object);
        }

        void Volume::Shader::update (const Displayable& object) 
        {
          clip1 = clip2 = clip3 = true;
          use_depth_testing = mode.use_depth_testing;
          Displayable::Shader::update (object);
        }











        void Volume::paint (Projection& projection)
        {
          // info for projection:
          int w = glarea()->width(), h = glarea()->height();
          float fov = FOV() / (float) (w+h);

          float depth = std::max (image()->interp.dim(0)*image()->interp.vox(0),
              std::max (image()->interp.dim(1)*image()->interp.vox(1),
                image()->interp.dim(2)*image()->interp.vox(2)));


          Math::Versor<float> Q = orientation();
          if (!Q) {
            Q = Math::Versor<float> (1.0, 0.0, 0.0, 0.0);
            set_orientation (Q);
          }

          // set up projection & modelview matrices:
          GL::mat4 P = GL::ortho (-w*fov, w*fov, -h*fov, h*fov, -depth, depth);
          GL::mat4 MV = adjust_projection_matrix (Q) * GL::translate  (-target()[0], -target()[1], -target()[2]);
          projection.set (MV, P);

          render_tools (projection, true);



          Point<> pos = image()->interp.voxel2scanner (Point<> (-0.5f, -0.5f, -0.5f));
          Point<> vec_X = image()->interp.voxel2scanner_dir (Point<> (image()->interp.dim(0), 0.0f, 0.0f));
          Point<> vec_Y = image()->interp.voxel2scanner_dir (Point<> (0.0f, image()->interp.dim(1), 0.0f));
          Point<> vec_Z = image()->interp.voxel2scanner_dir (Point<> (0.0f, 0.0f, image()->interp.dim(2)));
          GL::mat4 T2S;
          T2S(0,0) = vec_X[0];
          T2S(1,0) = vec_X[1];
          T2S(2,0) = vec_X[2];

          T2S(0,1) = vec_Y[0];
          T2S(1,1) = vec_Y[1];
          T2S(2,1) = vec_Y[2];

          T2S(0,2) = vec_Z[0];
          T2S(1,2) = vec_Z[1];
          T2S(2,2) = vec_Z[2];

          T2S(0,3) = pos[0];
          T2S(1,3) = pos[1];
          T2S(2,3) = pos[2];

          T2S(3,0) = T2S(3,1) = T2S(3,2) = 0.0f; 
          T2S(3,3) = 1.0f;
          GL::mat4 M = projection.modelview_projection() * T2S;

          int min_vox_index;
          if (image()->interp.vox(0) < image()->interp.vox (1)) 
            min_vox_index = image()->interp.vox(0) < image()->interp.vox (2) ? 0 : 2;
          else 
            min_vox_index = image()->interp.vox(1) < image()->interp.vox (2) ? 1 : 2;
          float step_size = 0.5 * image()->interp.vox(min_vox_index);
          Point<> ray = image()->interp.scanner2voxel_dir (projection.screen_normal() * step_size);
          ray[0] /= image()->interp.dim(0);
          ray[1] /= image()->interp.dim(1);
          ray[2] /= image()->interp.dim(2);



          if (!volume_VB || !volume_VAO || !volume_VI) {
            volume_VB.gen();
            volume_VI.gen();
            volume_VAO.gen();

            volume_VAO.bind();
            volume_VB.bind (GL_ARRAY_BUFFER);
            volume_VI.bind (GL_ELEMENT_ARRAY_BUFFER);

            glEnableVertexAttribArray (0);
            glVertexAttribPointer (0, 3, GL_BYTE, GL_FALSE, 0, (void*)0);

            GLbyte vertices[] = {
              0, 0, 0,
              0, 0, 1,
              0, 1, 0,
              0, 1, 1,
              1, 0, 0,
              1, 0, 1,
              1, 1, 0,
              1, 1, 1
            };
            glBufferData (GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
          }
          else {
            volume_VAO.bind();
            volume_VI.bind (GL_ELEMENT_ARRAY_BUFFER);
          }

          GLubyte indices[12];

          if (ray[0] < 0) {
            indices[0] = 4; 
            indices[1] = 5;
            indices[2] = 7;
            indices[3] = 6;
          }
          else {
            indices[0] = 0; 
            indices[1] = 1;
            indices[2] = 3;
            indices[3] = 2;
          }

          if (ray[1] < 0) {
            indices[4] = 2; 
            indices[5] = 3;
            indices[6] = 7;
            indices[7] = 6;
          }
          else {
            indices[4] = 0; 
            indices[5] = 1;
            indices[6] = 5;
            indices[7] = 4;
          }

          if (ray[2] < 0) {
            indices[8] = 1; 
            indices[9] = 3;
            indices[10] = 7;
            indices[11] = 5;
          }
          else {
            indices[8] = 0; 
            indices[9] = 2;
            indices[10] = 6;
            indices[11] = 4;
          }

          glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);

          image()->update_texture3D();
          image()->set_use_transparency (true);

          use_depth_testing = true;
          image()->start (volume_shader, image()->scaling_3D());
          glUniformMatrix4fv (glGetUniformLocation (volume_shader, "M"), 1, GL_FALSE, M);
          glUniform3fv (glGetUniformLocation (volume_shader, "ray"), 1, ray);
          glUniform1i (glGetUniformLocation (volume_shader, "image_sampler"), 0);

          glActiveTexture (GL_TEXTURE0);
          glBindTexture (GL_TEXTURE_3D, image()->texture3D_index());

          if (use_depth_testing) {

            glActiveTexture (GL_TEXTURE1);
            if (!depth_texture) {
              depth_texture.gen();
              depth_texture.bind (GL_TEXTURE_2D);
              glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
              glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
              glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
              glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
              glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
              glTexParameteri (GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_ALPHA);
            }
            else 
              depth_texture.bind (GL_TEXTURE_2D);

            glReadBuffer (GL_BACK);
            glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, projection.width(), projection.height(), 0);

            glUniform1i (glGetUniformLocation (volume_shader, "depth_sampler"), 1);


            GL::vec4 ray_eye = M * GL::vec4 (ray, 0.0);
            glUniform1f (glGetUniformLocation (volume_shader, "ray_z"), 0.5*ray_eye[2]);

            glEnable (GL_BLEND);
            glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          }



          glEnable (GL_DEPTH_TEST);
          glEnable (GL_TEXTURE_3D);
          glDepthMask (GL_FALSE);
          glActiveTexture (GL_TEXTURE0);

          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, image()->interpolate() ? GL_LINEAR : GL_NEAREST);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, image()->interpolate() ? GL_LINEAR : GL_NEAREST);

          glDrawElements (GL_QUADS, 12, GL_UNSIGNED_BYTE, (void*) 0);
          image()->stop (volume_shader);

          glDisable (GL_TEXTURE_3D);
          glDisable (GL_BLEND);

          draw_crosshairs (projection);
          draw_orientation_labels (projection);
        }






      }
    }
  }
}



