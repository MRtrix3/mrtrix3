/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "gui/mrview/mode/ortho.h"

#include "mrtrix.h"
#include "gui/cursor.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {




        void Ortho::paint (Projection& projection)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          // set up OpenGL environment:
          gl::Disable (gl::BLEND);
          gl::Disable (gl::DEPTH_TEST);
          gl::DepthMask (gl::FALSE_);
          gl::ColorMask (gl::TRUE_, gl::TRUE_, gl::TRUE_, gl::TRUE_);

          GLint w = width()/2;
          GLint h = height()/2;

          // Depth test state may have been altered after drawing each plane
          // so need to guarantee depth test is off for subsequent plane.
          // Ideally, state should be restored by callee but this is safer

          projections[0].set_viewport (window(), w, h, w, h); 
          draw_plane (0, slice_shader, projections[0]);

          gl::Disable (gl::DEPTH_TEST);
          projections[1].set_viewport (window(), 0, h, w, h); 
          draw_plane (1, slice_shader, projections[1]);

          gl::Disable (gl::DEPTH_TEST);
          projections[2].set_viewport (window(), 0, 0, w, h); 
          draw_plane (2, slice_shader, projections[2]);

          projection.set_viewport (window());

          GL::mat4 MV = GL::identity();
          GL::mat4 P = GL::ortho (0, width(), 0, height(), -1.0, 1.0);
          projection.set (MV, P);

          gl::Disable (gl::DEPTH_TEST);

          if (!frame_VB || !frame_VAO) {
            frame_VB.gen();
            frame_VAO.gen();

            frame_VB.bind (gl::ARRAY_BUFFER);
            frame_VAO.bind();

            gl::EnableVertexAttribArray (0);
            gl::VertexAttribPointer (0, 2, gl::FLOAT, gl::FALSE_, 0, (void*)0);

            GLfloat data [] = {
              -1.0f, 0.0f,
              1.0f, 0.0f,
              0.0f, -1.0f,
              0.0f, 1.0f
            };
            gl::BufferData (gl::ARRAY_BUFFER, sizeof(data), data, gl::STATIC_DRAW);
          }
          else 
            frame_VAO.bind();

          if (!frame_program) {
            GL::Shader::Vertex vertex_shader (
                "layout(location=0) in vec2 pos;\n"
                "void main () {\n"
                "  gl_Position = vec4 (pos, 0.0, 1.0);\n"
                "}\n");
            GL::Shader::Fragment fragment_shader (
                "out vec3 color;\n"
                "void main () {\n"
                "  color = vec3 (0.1);\n"
                "}\n");
            frame_program.attach (vertex_shader);
            frame_program.attach (fragment_shader);
            frame_program.link();
          }

          frame_program.start();
          gl::DrawArrays (gl::LINES, 0, 4);
          frame_program.stop();

          gl::Enable (gl::DEPTH_TEST);
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }





        const Projection* Ortho::get_current_projection () const  
        {
          if (current_plane < 0 || current_plane > 2)
            return NULL;
          return &projections[current_plane];
        }



        void Ortho::mouse_press_event ()
        {
          if (window().mouse_position().x() < width()/2) 
            if (window().mouse_position().y() >= height()/2) 
              current_plane = 1;
            else 
              current_plane = 2;
          else 
            if (window().mouse_position().y() >= height()/2)
              current_plane = 0;
            else 
              current_plane = -1;
        }




        void Ortho::slice_move_event (float x) 
        {
          if (window().active_camera_interactor() && window().active_camera_interactor()->slice_move_event(x))
            return;

          const Projection* proj = get_current_projection();
          if (!proj) return;
          const auto &header = image()->header();
          float increment = snap_to_image() ?
            x * header.spacing (current_plane) :
            x * std::pow (header.spacing(0) * header.spacing(1) * header.spacing(2), 1/3.f);
          auto move = get_through_plane_translation (increment, *proj);

          set_focus (focus() + move);
          updateGL();
        }


        void Ortho::panthrough_event ()
        {
          if (window().active_camera_interactor() && window().active_camera_interactor()->panthrough_event())
            return;

          const Projection* proj = get_current_projection();
          if (!proj) return;
          auto move = get_through_plane_translation_FOV (window().mouse_displacement().y(), *proj);

          set_focus (focus() + move);
          updateGL();
        }


      }
    }
  }
}



