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
#include "gui/mrview/tool/view.h"

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
          std::string source = 
            "layout(location=0) in vec3 vertpos;\n"
            "uniform mat4 M;\n"
            "out vec3 texcoord;\n";

          for (int n = 0; n < mode.overlays_for_3D.size(); ++n) 
            source += 
              "uniform mat4 overlay_M" + str(n) + ";\n"
              "out vec3 overlay_texcoord" + str(n) + ";\n";

          source += 
            "void main () {\n"
            "  texcoord = vertpos;\n"
            "  gl_Position =  M * vec4 (vertpos,1);\n";

          for (int n = 0; n < mode.overlays_for_3D.size(); ++n) 
            source += 
              "  overlay_texcoord"+str(n) + " = (overlay_M"+str(n) + " * vec4 (vertpos,1)).xyz;\n";

          source +=
            "}\n";

          return source;
        }






        std::string Volume::Shader::fragment_shader_source (const Displayable& object)
        {
          std::string source = object.declare_shader_variables() +
            "uniform sampler3D image_sampler;\n"
            "in vec3 texcoord;\n";

          if (clip[0]) source += "uniform vec4 clip0;\n";
          if (clip[1]) source += "uniform vec4 clip1;\n";
          if (clip[2]) source += "uniform vec4 clip2;\n";

          for (int n = 0; n < mode.overlays_for_3D.size(); ++n) {
            source += mode.overlays_for_3D[n]->declare_shader_variables ("overlay"+str(n)+"_") +
              "uniform sampler3D overlay_sampler"+str(n) + ";\n"
              "uniform vec3 overlay_ray"+str(n) + ";\n"
              "in vec3 overlay_texcoord"+str(n) + ";\n";
          }

          source +=
            "uniform sampler2D depth_sampler;\n"
            "uniform mat4 M;\n"
            "uniform float ray_z;\n"
            "uniform vec3 ray;\n"
            "out vec4 final_color;\n"
            "void main () {\n"
            "float amplitude;\n"
            "vec4 color;\n";


          source += 
            "  final_color = vec4 (0.0);\n"
            "  float dither = fract(sin(gl_FragCoord.x * 12.9898 + gl_FragCoord.y * 78.233) * 43758.5453);\n"
            "  vec3 coord = texcoord + ray * dither;\n";

          for (int n = 0; n < mode.overlays_for_3D.size(); ++n) 
            source += 
              "  vec3 overlay_coord"+str(n) +" = overlay_texcoord"+str(n) + " + overlay_ray"+str(n) + " * dither;\n";

              source +=
            "  float depth = texelFetch (depth_sampler, ivec2(gl_FragCoord.xy), 0).r;\n"
            "  float current_depth = gl_FragCoord.z + ray_z * dither;\n"
            "  int nmax = 10000;\n"
            "  if (ray.x < 0.0) nmax = int (-texcoord.s/ray.x);\n"
            "  else if (ray.x > 0.0) nmax = int ((1.0-texcoord.s) / ray.x);\n"
            "  if (ray.y < 0.0) nmax = min (nmax, int (-texcoord.t/ray.y));\n"
            "  else if (ray.y > 0.0) nmax = min (nmax, int ((1.0-texcoord.t) / ray.y));\n"
            "  if (ray.z < 0.0) nmax = min (nmax, int (-texcoord.p/ray.z));\n"
            "  else if (ray.z > 0.0) nmax = min (nmax, int ((1.0-texcoord.p) / ray.z));\n"
            "  nmax = min (nmax, int ((depth - current_depth) / ray_z));\n"
            "  if (nmax <= 0) return;\n"
            "  for (int n = 0; n < nmax; ++n) {\n"
            "    coord += ray;\n";

          if (clip[0] || clip[1] || clip[2]) {
            source += "    bool show = true;\n";
            if (clip[0]) source += "    if (dot (coord, clip0.xyz) > clip0.w)\n";
            if (clip[1]) source += "      if (dot (coord, clip1.xyz) > clip1.w)\n";
            if (clip[2]) source += "        if (dot (coord, clip2.xyz) > clip2.w)\n";
            source += 
              "          show = false;\n"
              "    if (show) {\n";
          }

          source += 
            "      color = texture (image_sampler, coord);\n"
            "      amplitude = " + std::string (ColourMap::maps[object.colourmap].amplitude) + ";\n"
            "      if (!isnan(amplitude) && !isinf(amplitude)";

          if (object.use_discard_lower())
            source += " && amplitude >= lower";

          if (object.use_discard_upper())
            source += " && amplitude <= upper";

          source += " && amplitude >= alpha_offset) {\n"
            "        color.a = clamp ((amplitude - alpha_offset) * alpha_scale, 0, alpha);\n";

          if (!ColourMap::maps[object.colourmap].special) {
            source += 
              "        amplitude = clamp (";
            if (object.scale_inverted()) 
              source += "1.0 -";
            source += 
              " scale * (amplitude - offset), 0.0, 1.0);\n";
          }

          source += 
            std::string ("        ") + ColourMap::maps[object.colourmap].mapping +
            "        final_color.rgb += (1.0 - final_color.a) * color.rgb * color.a;\n"
            "        final_color.a += color.a;\n"
            "      }\n";

          if (clip[0] || clip[1] || clip[2]) 
            source += "    }\n";




/*          // OVERLAYS:
          for (int n = 0; n < mode.overlays_for_3D.size(); ++n) {
            source += 
              "    overlay_coord"+str(n) + " += overlay_ray"+str(n) + ";\n"
              "    color = texture (overlay_sampler"+str(n) +", overlay_coord"+str(n) +");\n"
              "    amplitude = " + std::string (ColourMap::maps[mode.overlays_for_3D[n]->colourmap].amplitude) + ";\n"
              "    if (!isnan(amplitude) && !isinf(amplitude)";

            if (mode.overlays_for_3D[n]->use_discard_lower())
              source += " && amplitude >= overlay"+str(n)+"_lower";

            if (mode.overlays_for_3D[n]->use_discard_upper())
              source += " && amplitude <= overlay"+str(n)+"_upper";

            source += ") {\n";

            if (!ColourMap::maps[mode.overlays_for_3D[n]->colourmap].special) {
              source += 
                "      amplitude = clamp (";
              if (mode.overlays_for_3D[n]->scale_inverted()) 
                source += "1.0 -";
              source += 
                " overlay"+str(n)+"_scale * (amplitude - overlay"+str(n)+"_offset), 0.0, 1.0);\n";
            }

            source += 
              std::string ("      ") + ColourMap::maps[mode.overlays_for_3D[n]->colourmap].mapping +
              "      final_color.rgb += (1.0 - final_color.a) * color.rgb * color.a;\n"
              "      final_color.a += amplitude * overlay"+str(n) + "_alpha;\n"
              "    }\n";
          }
*/


          source += 
            "    if (final_color.a > 0.95) break;\n"
            "  }\n"
            "}\n";

          return source;
        }




        bool Volume::Shader::need_update (const Displayable& object) const 
        {
          if (clip[0] != mode.do_clip (0)) return true;
          if (clip[1] != mode.do_clip (1)) return true;
          if (clip[2] != mode.do_clip (2)) return true;
          if (mode.update_overlays) return true;
          return Displayable::Shader::need_update (object);
        }

        void Volume::Shader::update (const Displayable& object) 
        {
          clip[0] = mode.do_clip (0);
          clip[1] = mode.do_clip (1);
          clip[2] = mode.do_clip (2);
          Displayable::Shader::update (object);
        }





        namespace {

          inline GL::vec4 clip_real2tex (const GL::mat4& T2S, const GL::mat4& S2T, const GL::vec4& plane) 
          {
            GL::vec4 normal = T2S * GL::vec4 (plane[0], plane[1], plane[2], 0.0);
            GL::vec4 on_plane = S2T * GL::vec4 (plane[3]*plane[0], plane[3]*plane[1], plane[3]*plane[2], 1.0);
            normal[3] = on_plane[0]*normal[0] + on_plane[1]*normal[1] + on_plane[2]*normal[2];
            return normal;
          }


          inline GL::mat4 get_tex_to_scanner_matrix (const Image& image)
          {
            Point<> pos = image.interp.voxel2scanner (Point<> (-0.5f, -0.5f, -0.5f));
            Point<> vec_X = image.interp.voxel2scanner_dir (Point<> (image.interp.dim(0), 0.0f, 0.0f));
            Point<> vec_Y = image.interp.voxel2scanner_dir (Point<> (0.0f, image.interp.dim(1), 0.0f));
            Point<> vec_Z = image.interp.voxel2scanner_dir (Point<> (0.0f, 0.0f, image.interp.dim(2)));
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

            return T2S;
          }

        }


        inline bool Volume::do_clip (int n) const
        {
          Tool::Dock* dock = dynamic_cast<Tool::__Action__*>(window.tools()->actions()[0])->dock;
          if (!dock) 
            return false;

          Tool::View* view_tool = dynamic_cast<Tool::View*> (dock->tool);
          return view_tool->clip_on_button[n]->isChecked();
        }




        inline bool Volume::edit_clip (int n) const
        {
          Tool::Dock* dock = dynamic_cast<Tool::__Action__*>(window.tools()->actions()[0])->dock;
          if (!dock) 
            return false;

          return dynamic_cast<Tool::View*> (dock->tool)->clip_edit_button[n]->isChecked();
        }





        inline bool Volume::editing () const
        {
          Tool::Dock* dock = dynamic_cast<Tool::__Action__*>(window.tools()->actions()[0])->dock;
          if (!dock) 
            return false;

          Tool::View* view_tool = dynamic_cast<Tool::View*> (dock->tool);
          return view_tool->clip_edit_button[0]->isChecked() || view_tool->clip_edit_button[1]->isChecked() || view_tool->clip_edit_button[2]->isChecked();
        }





        void Volume::reset_clip_planes() 
        {
          clip[0] = GL::vec4 (0.0, 0.0, 1.0, 0.0);
          clip[1] = GL::vec4 (1.0, 0.0, 0.0, 0.0);
          clip[2] = GL::vec4 (0.0, 1.0, 0.0, 0.0);
        }



        void Volume::invert_clip_plane (int n) 
        {
          clip[n][0] = -clip[n][0];
          clip[n][1] = -clip[n][1];
          clip[n][2] = -clip[n][2];
          clip[n][3] = -clip[n][3];
          updateGL();
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



          overlays_for_3D.clear();
          render_tools (projection, true);
          glDisable (GL_BLEND);
          glEnable (GL_DEPTH_TEST);
          glDepthMask (GL_TRUE);

          draw_crosshairs (projection);




          GL::mat4 T2S = get_tex_to_scanner_matrix (*image());
          GL::mat4 M = projection.modelview_projection() * T2S;
          GL::mat4 S2T = GL::inv (T2S);

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

          image()->start (volume_shader, image()->scaling_3D());
          glUniformMatrix4fv (glGetUniformLocation (volume_shader, "M"), 1, GL_FALSE, M);
          glUniform3fv (glGetUniformLocation (volume_shader, "ray"), 1, ray);
          glUniform1i (glGetUniformLocation (volume_shader, "image_sampler"), 0);

          glActiveTexture (GL_TEXTURE0);
          glBindTexture (GL_TEXTURE_3D, image()->texture3D_index());


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

          if (volume_shader.clip[0]) 
            glUniform4fv (glGetUniformLocation (volume_shader, "clip0"), 1, clip_real2tex (T2S, S2T, clip[0]));
          if (volume_shader.clip[1]) 
            glUniform4fv (glGetUniformLocation (volume_shader, "clip1"), 1, clip_real2tex (T2S, S2T, clip[1]));
          if (volume_shader.clip[2]) 
            glUniform4fv (glGetUniformLocation (volume_shader, "clip2"), 1, clip_real2tex (T2S, S2T, clip[2]));

          for (int n = 0; n < overlays_for_3D.size(); ++n) {
            overlays_for_3D[n]->update_texture3D();
            GL::mat4 overlay_M = GL::inv (get_tex_to_scanner_matrix (*overlays_for_3D[n])) * T2S;
            GL::vec4 overlay_ray = overlay_M * GL::vec4 (ray, 0.0);
            glUniformMatrix4fv (glGetUniformLocation (volume_shader, ("overlay_M"+str(n)).c_str()), 1, GL_FALSE, overlay_M);
            glUniform3fv (glGetUniformLocation (volume_shader, ("overlay_ray"+str(n)).c_str()), 1, overlay_ray);

            glActiveTexture (GL_TEXTURE2 + n);
            glBindTexture (GL_TEXTURE_3D, overlays_for_3D[n]->texture3D_index());
            glUniform1i (glGetUniformLocation (volume_shader, ("overlay_sampler"+str(n)).c_str()), 2+n);
            overlays_for_3D[n]->start (volume_shader, overlays_for_3D[n]->scaling_3D(), "overlay"+str(n)+"_");
            glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, overlays_for_3D[n]->interpolate() ? GL_LINEAR : GL_NEAREST);
            glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, overlays_for_3D[n]->interpolate() ? GL_LINEAR : GL_NEAREST);
          }

          GL::vec4 ray_eye = M * GL::vec4 (ray, 0.0);
          glUniform1f (glGetUniformLocation (volume_shader, "ray_z"), 0.5*ray_eye[2]);

          glEnable (GL_BLEND);
          glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

          glDepthMask (GL_FALSE);
          glActiveTexture (GL_TEXTURE0);

          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, image()->interpolate() ? GL_LINEAR : GL_NEAREST);
          glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, image()->interpolate() ? GL_LINEAR : GL_NEAREST);

          glDrawElements (GL_QUADS, 12, GL_UNSIGNED_BYTE, (void*) 0);
          image()->stop (volume_shader);

          glDisable (GL_BLEND);

          draw_orientation_labels (projection);
        }





        inline void Volume::move_clip_planes_in_out (float distance) 
        {
          Point<> d = get_current_projection()->screen_normal();
          for (size_t n = 0; n < 3; ++n) {
            if (edit_clip(n)) 
              clip[n][3] += distance * (clip[n][0]*d[0] + clip[n][1]*d[1] + clip[n][2]*d[2]);
          }
          updateGL();
        }


        inline void Volume::rotate_clip_planes (const Math::Versor<float>& rot)
        {
          for (size_t n = 0; n < 3; ++n) {
            if (edit_clip(n)) {
              float distance_to_focus = clip[n][0]*focus()[0] + clip[n][1]*focus()[1] + clip[n][2]*focus()[2] - clip[n][3];
              Math::Versor<float> norm (0.0f, clip[n][0], clip[n][1], clip[n][2]);
              Math::Versor<float> rotated = norm * rot;
              rotated.normalise();
              clip[n][0] = rotated[1];
              clip[n][1] = rotated[2];
              clip[n][2] = rotated[3];
              clip[n][3] = clip[n][0]*focus()[0] + clip[n][1]*focus()[1] + clip[n][2]*focus()[2] - distance_to_focus;
            }
          }
          updateGL();
        }





        void Volume::slice_move_event (int x) 
        {
          if (editing()) 
            move_clip_planes_in_out (x * std::min (std::min (image()->header().vox(0), image()->header().vox(1)), image()->header().vox(2)));
          else
            Base::slice_move_event (x);
        }



        void Volume::pan_event () 
        {
          if (editing()) {
            Point<> move = get_current_projection()->screen_to_model_direction (window.mouse_displacement(), target());
            for (size_t n = 0; n < 3; ++n) {
              if (edit_clip(n)) 
                clip[n][3] += (clip[n][0]*move[0] + clip[n][1]*move[1] + clip[n][2]*move[2]);
            }
            updateGL();
          }
          else 
            Base::pan_event();
        }


        void Volume::panthrough_event () 
        {
          if (editing())
            move_clip_planes_in_out (MOVE_IN_OUT_FOV_MULTIPLIER * window.mouse_displacement().y() * FOV());
          else
            Base::panthrough_event();
        }



        void Volume::tilt_event () 
        {
          if (editing()) {
            Math::Versor<float> rot = get_tilt_rotation();
            if (!rot) 
              return;
            rotate_clip_planes (rot);
          }
          else 
            Base::tilt_event();
        }



        void Volume::rotate_event () 
        {
          if (editing()) {
            Math::Versor<float> rot = get_rotate_rotation();
            if (!rot) 
              return;
            rotate_clip_planes (rot);
          }
          else 
            Base::rotate_event();
        }


      }
    }
  }
}



