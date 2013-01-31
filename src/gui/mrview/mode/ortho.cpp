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

#include <QAction>

#include "mrtrix.h"
#include "gui/cursor.h"
#include "math/vector.h"
#include "gui/mrview/mode/ortho.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        Ortho::Ortho (Window& parent) : 
          Base (parent),
          projections (3, projection),
          current_plane (-1),
          vertex_buffer_ID (0),
          vertex_array_object_ID (0) {
          }

        Ortho::~Ortho () { 
          if (vertex_buffer_ID)
            glDeleteBuffers (1, &vertex_buffer_ID);
          if (vertex_array_object_ID)
            glDeleteVertexArrays (1, &vertex_array_object_ID);
        }




        void Ortho::paint ()
        {
          if (!focus()) reset_view();
          if (!target()) set_target (focus());

          GLint w = glarea()->width()/2;
          GLint h = glarea()->height()/2;
          float fov = FOV() / float(w+h);
          float fovx = w * fov;
          float fovy = h * fov;

          draw_plane (0, w, h, fovx, fovy);
          draw_plane (1, w, h, fovx, fovy);
          draw_plane (2, w, h, fovx, fovy);

          projection.set_viewport (0, 0, glarea()->width(), glarea()->height());

          GL::mat4 MV = GL::identity();
          GL::mat4 P = GL::ortho (0, glarea()->width(), 0, glarea()->height(), -1.0, 1.0);
          projection.set (MV, P);

          glDisable (GL_DEPTH_TEST);
          glColor4f (0.1, 0.1, 0.1, 1.0);
          glLineWidth (2.0);

          if (!vertex_buffer_ID || !vertex_array_object_ID) {
            glGenBuffers (1, &vertex_buffer_ID);
            glGenVertexArrays (1, &vertex_array_object_ID);

            glBindBuffer (GL_ARRAY_BUFFER, vertex_buffer_ID);
            glBindVertexArray (vertex_array_object_ID);

            glEnableVertexAttribArray (0);
            glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

            GLfloat data [] = {
              -1.0f, 0.0f,
              1.0f, 0.0f,
              0.0f, -1.0f,
              0.0f, 1.0f
            };
            glBufferData (GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
          }
          else 
            glBindVertexArray (vertex_array_object_ID);

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
          glDrawArrays (GL_LINES, 0, 4);
          frame_program.stop();

          glEnable (GL_DEPTH_TEST);
        }





        void Ortho::draw_plane (int axis, int w, int h, float fovx, float fovy)
        {
          // image slice:
          Point<> voxel (image()->interp.scanner2voxel (focus()));
          int slice = Math::round (voxel[axis]);

          // camera target:
          Point<> F = image()->interp.scanner2voxel (target());
          F[axis] = slice;
          F = image()->interp.voxel2scanner (F);

          // info for projection:
          float depth = image()->interp.dim (axis) * image()->interp.vox (axis);

          switch (axis) {
            case 0: projections[axis].set_viewport (w, h, w, h); break;
            case 1: projections[axis].set_viewport (0, h, w, h); break;
            case 2: projections[axis].set_viewport (0, 0, w, h); break;
          }

          // set up modelview and projection matrices:
          GL::mat4 MV = adjust_projection_matrix (image()->interp.image2scanner_matrix(), axis) * GL::translate (-F[0], -F[1], -F[2]);
          GL::mat4 P = GL::ortho (-fovx, fovx, -fovy, fovy, -depth, depth);
          projections[axis].set (MV, P);

          // set up OpenGL environment:
          glDisable (GL_BLEND);
          glEnable (GL_TEXTURE_2D);
          glShadeModel (GL_FLAT);
          glDisable (GL_DEPTH_TEST);
          glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDepthMask (GL_FALSE);
          glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

          // render image:
          image()->render2D (projections[axis], axis, slice);

          glDisable (GL_TEXTURE_2D);

          render_tools2D (projections[axis]);

          if (window.show_crosshairs()) 
            projections[axis].render_crosshairs (focus());

          if (window.show_orientation_labels()) {
            projections[axis].setup_render_text (1.0, 0.0, 0.0);
            switch (axis) {
              case 0:
                projections[axis].render_text ("A", LeftEdge);
                projections[axis].render_text ("S", TopEdge);
                projections[axis].render_text ("P", RightEdge);
                projections[axis].render_text ("I", BottomEdge);
                break;
              case 1:
                projections[axis].render_text ("R", LeftEdge);
                projections[axis].render_text ("S", TopEdge);
                projections[axis].render_text ("L", RightEdge);
                projections[axis].render_text ("I", BottomEdge);
                break;
              case 2:
                projections[axis].render_text ("R", LeftEdge);
                projections[axis].render_text ("A", TopEdge);
                projections[axis].render_text ("L", RightEdge);
                projections[axis].render_text ("P", BottomEdge);
                break;
              default:
                assert (0);
            }
            projections[axis].done_render_text();
          }

        }




        void Ortho::reset_event ()
        {
          reset_view();
          updateGL();
        }



        void Ortho::mouse_press_event ()
        {
          if (window.mouse_position().x() < glarea()->width()/2) 
            if (window.mouse_position().y() >= glarea()->height()/2) 
              current_plane = 1;
            else 
              current_plane = 2;
          else 
            if (window.mouse_position().y() >= glarea()->height()/2)
              current_plane = 0;
            else 
              current_plane = -1;
        }




        void Ortho::slice_move_event (int x) 
        { 
          if (current_plane < 0) return;
          set_focus (focus() + move_in_out_displacement (x * image()->header().vox (current_plane),projections[current_plane]));
          updateGL();
        } 


        void Ortho::set_focus_event ()
        {
          if (current_plane < 0) 
            return;
          Base::set_focus (projections[current_plane].screen_to_model (window.mouse_position(), focus()));
          updateGL();
        }


        void Ortho::contrast_event ()
        {
          image()->adjust_windowing (window.mouse_displacement());
          window.on_scaling_changed();
          updateGL();
        }






        void Ortho::pan_event () 
        {
          if (current_plane < 0) return;
          set_target (target() - projections[current_plane].screen_to_model_direction (window.mouse_displacement(), target()));
          updateGL();
        }


        void Ortho::panthrough_event () 
        { 
          if (current_plane < 0) return;
          set_focus (focus() + move_in_out_displacement (window.mouse_displacement().y(), projections[current_plane]));
          updateGL();
        }




        void Ortho::reset_view ()
        {
          if (!image()) return;
          float dim[] = {
            image()->header().dim (0)* image()->header().vox (0),
            image()->header().dim (1)* image()->header().vox (1),
            image()->header().dim (2)* image()->header().vox (2)
          };

          Point<> p (image()->header().dim (0) /2.0, image()->header().dim (1) /2.0, image()->header().dim (2) /2.0);
          Base::set_focus (image()->interp.voxel2scanner (p));

          set_FOV (std::max (dim[0], std::max (dim[1], dim[2])));

          set_target (Point<>());
        }




      }
    }
  }
}



