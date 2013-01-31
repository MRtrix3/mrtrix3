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

#include "gui/projection.h"

namespace MR
{
  namespace GUI
  {

    void Projection::render_crosshairs (const Point<>& focus)
    {
      if (!vertex_buffer_ID || !vertex_array_object_ID) {
        glGenBuffers (1, &vertex_buffer_ID);
        glGenVertexArrays (1, &vertex_array_object_ID);

        glBindBuffer (GL_ARRAY_BUFFER, vertex_buffer_ID);
        glBindVertexArray (vertex_array_object_ID);

        glEnableVertexAttribArray (0);
        glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
      }
      else {
        glBindBuffer (GL_ARRAY_BUFFER, vertex_buffer_ID);
        glBindVertexArray (vertex_array_object_ID);
      }

      if (!crosshairs_program) {
        GL::Shader::Vertex vertex_shader (
            "layout(location=0) in vec2 pos;\n"
            "void main () {\n"
            "  gl_Position = vec4 (pos, 0.0, 1.0);\n"
            "}\n");
        GL::Shader::Fragment fragment_shader (
            "out vec4 color;\n"
            "void main () {\n"
            "  color = vec4 (1.0, 1.0, 0.0, 0.5);\n"
            "}\n");
        crosshairs_program.attach (vertex_shader);
        crosshairs_program.attach (fragment_shader);
        crosshairs_program.link();
      }

      Point<> F = model_to_screen (focus);
      F[0] -= x_position();
      F[1] -= y_position();
      
      F[0] = 2.0f * F[0] / width() - 1.0f;
      F[1] = 2.0f * F[1] / height() - 1.0f;

      GLfloat data [] = {
        F[0], -1.0f,
        F[0], 1.0f,
        -1.0f, F[1],
        1.0f, F[1]
      };
      glBufferData (GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

      glDepthMask (GL_FALSE);
      glLineWidth (1.0);
      glEnable (GL_BLEND);
      glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      crosshairs_program.start();
      glDrawArrays (GL_LINES, 0, 4);
      crosshairs_program.stop();
    }




    Projection::~Projection () 
    {
      if (vertex_buffer_ID)
        glDeleteBuffers (1, &vertex_buffer_ID);
      if (vertex_array_object_ID)
        glDeleteVertexArrays (1, &vertex_array_object_ID);
    }

  }
}

