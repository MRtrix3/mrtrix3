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

        Volume::Volume (Window& parent) : 
          Base (parent, FocusContrast | MoveTarget | TiltRotate | ShaderTransparency),
          shader_flags (0xFFFFFFFF),
          shader_colourmap (0) {
          }






        void Volume::recompile_shader () 
        {
          if (volume_program)
            volume_program.clear();

          shader_flags = image()->flags();
          shader_colourmap = image()->colourmap();

          GL::Shader::Vertex vertex_shader (
              "layout(location=0) in vec3 vertpos;\n"
              "uniform mat4 M;\n"
              "out vec3 texcoord;\n"
              "void main () {\n"
              "  texcoord = vertpos;\n"
              "  gl_Position =  M * vec4 (vertpos,1);\n"
              "}\n");



          std::string fragment_shader_source = 
            "uniform sampler3D tex;\n"
            "in vec3 texcoord;\n"
            "uniform float offset, scale, alpha_scale, alpha_offset, alpha";
          if (image()->flags() & DiscardLower) fragment_shader_source += ", lower";
          if (image()->flags() & DiscardUpper) fragment_shader_source += ", upper";

            fragment_shader_source += ";\n"
            "uniform vec3 ray;\n"
            "out vec4 final_color;\n"
            "void main () {\n"
            "  final_color = vec4 (0.0);\n"
            "  vec3 coord = texcoord + ray * (fract(sin(gl_FragCoord.x * 12.9898 + gl_FragCoord.y * 78.233) * 43758.5453));\n"
            "  int nmax = 10000;\n"
            "  if (ray.x < 0.0) nmax = int (-texcoord.s/ray.x);\n"
            "  else if (ray.x > 0.0) nmax = int ((1.0-texcoord.s) / ray.x);\n"
            "  if (ray.y < 0.0) nmax = min (nmax, int (-texcoord.t/ray.y));\n"
            "  else if (ray.y > 0.0) nmax = min (nmax, int ((1.0-texcoord.t) / ray.y));\n"
            "  if (ray.z < 0.0) nmax = min (nmax, int (-texcoord.p/ray.z));\n"
            "  else if (ray.z > 0.0) nmax = min (nmax, int ((1.0-texcoord.p) / ray.z));\n"
            "  for (int n = 0; n < nmax; ++n) {\n"
            "    coord += ray;\n"
            "    vec4 color = texture (tex, coord);\n"
            "    float amplitude = " + std::string (ColourMap::maps[image()->colourmap()].amplitude) + ";\n"
            "    if (isnan(amplitude) || isinf(amplitude)) continue;\n";

          if (image()->flags() & DiscardLower)
            fragment_shader_source += 
              "    if (amplitude < lower) continue;\n";

          if (image()->flags() & DiscardUpper)
            fragment_shader_source += 
              "    if (amplitude > upper) continue;\n";

          fragment_shader_source +=
            "    if (amplitude < alpha_offset) continue;\n"
            "    color.a = clamp ((amplitude - alpha_offset) * alpha_scale, 0, alpha);\n";

          if (!ColourMap::maps[image()->colourmap()].special) {
            fragment_shader_source += 
              "    amplitude = clamp (";
            if (image()->flags() & InvertScale) fragment_shader_source += "1.0 -";
            fragment_shader_source += 
              " scale * (amplitude - offset), 0.0, 1.0);\n";
          }

          fragment_shader_source += 
            std::string ("    ") + ColourMap::maps[image()->colourmap()].mapping +
            "    final_color.rgb += (1.0 - final_color.a) * color.rgb * color.a;\n"
            "    final_color.a += color.a;\n" 
            "    if (final_color.a > 0.95) break;\n"
            "  }\n"
            "}\n";

          GL::Shader::Fragment fragment_shader (fragment_shader_source);

          volume_program.attach (vertex_shader);
          volume_program.attach (fragment_shader);
          volume_program.link();
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
          GL::mat4 M = projection.modelview_projection() * GL::mat4 (T2S);
          
          float step_size = std::min (std::min (
                image()->interp.vox(0) / float (image()->interp.dim(0)),
                image()->interp.vox(1) / float (image()->interp.dim(1))),
                image()->interp.vox(2) / float (image()->interp.dim(2)));
          Point<> ray = image()->interp.scanner2voxel_dir (projection.screen_normal());
          ray[0] /= image()->interp.dim(0);
          ray[1] /= image()->interp.dim(1);
          ray[2] /= image()->interp.dim(2);
          ray.normalise();
          ray *= step_size;

          if (!volume_program || shader_flags != image()->flags() || shader_colourmap != image()->colourmap()) 
            recompile_shader();

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
          if (!finite (image()->alpha) || !finite (image()->transparent_intensity) || !finite (image()->opaque_intensity)) {
            image()->alpha = 0.5;
            image()->transparent_intensity = image()->display_midpoint - 0.4f * image()->display_range;
            image()->opaque_intensity = image()->display_midpoint;
          }

          volume_program.start();

          glUniformMatrix4fv (glGetUniformLocation (volume_program, "M"), 1, GL_FALSE, M);
          glUniform3fv (glGetUniformLocation (volume_program, "ray"), 1, ray);
          glUniform1f (glGetUniformLocation (volume_program, "offset"), (image()->display_midpoint - 0.5f * image()->display_range) / image()->scaling_3D());
          glUniform1f (glGetUniformLocation (volume_program, "scale"), image()->scaling_3D() / image()->display_range);
          if (image()->flags() & DiscardLower)
            glUniform1f (glGetUniformLocation (volume_program, "lower"), image()->lessthan / image()->scaling_3D());
          if (image()->flags() & DiscardUpper)
            glUniform1f (glGetUniformLocation (volume_program, "upper"), image()->greaterthan / image()->scaling_3D());
          glUniform1f (glGetUniformLocation (volume_program, "alpha_scale"), image()->scaling_3D() / (image()->opaque_intensity - image()->transparent_intensity));
          glUniform1f (glGetUniformLocation (volume_program, "alpha_offset"), image()->transparent_intensity / image()->scaling_3D());
          glUniform1f (glGetUniformLocation (volume_program, "alpha"), image()->alpha);

          glEnable (GL_DEPTH_TEST);
          glEnable (GL_TEXTURE_3D);
          glDepthMask (GL_FALSE);

          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, image()->interpolate() ? GL_LINEAR : GL_NEAREST);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, image()->interpolate() ? GL_LINEAR : GL_NEAREST);

          glDrawElements (GL_QUADS, 12, GL_UNSIGNED_BYTE, (void*) 0);
          volume_program.stop();

          glDisable (GL_TEXTURE_3D);
          glDisable (GL_BLEND);

          draw_crosshairs (projection);
          draw_orientation_labels (projection);
        }






      }
    }
  }
}



