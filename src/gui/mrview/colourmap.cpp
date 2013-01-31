/*
   Copyright 2010 Brain Research Institute, Melbourne, Australia

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

#include "gui/opengl/font.h"
#include "gui/mrview/colourmap.h"
#include "gui/mrview/displayable.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace ColourMap
      {

        const char* Entry::default_amplitude = "color.r";



        const Entry maps[] = {
          Entry ("Gray", 
              "color.rgb = vec3 (amplitude);\n"),

          Entry ("Hot", 
              "color.rgb = vec3 (2.7213 * amplitude, 2.7213 * amplitude - 1.0, 3.7727 * amplitude - 2.7727);\n"),

          Entry ("Jet", 
              "color.rgb = 1.5 - 4.0 * abs (amplitude - vec3(0.25, 0.5, 0.75));\n"),

          Entry ("RGB", 
              "color.rgb = scale * (abs(color.rgb) - offset);\n",
              "length (color.rgb)", 
              true),

          Entry ("Complex", 
              "float phase = atan (color.r, color.g) * 0.954929658551372;\n"
              "color.rgb = phase + vec3 (-2.0, 0.0, 2.0);\n"
              "if (phase > 2.0) color.b -= 6.0;\n"
              "if (phase < -2.0) color.r += 6.0;\n"
              "color.rgb = clamp (scale * (amplitude - offset), 0.0, 1.0) * (2.0 - abs (color.rgb));\n", 
              "length (color.rg)", 
              true),

          Entry (NULL, NULL, NULL, true)
        };





        void create_menu (QWidget* parent, QActionGroup*& group, QMenu* menu, QAction** & actions, bool create_shortcuts)
        {
          group = new QActionGroup (parent);
          group->setExclusive (true);
          actions = new QAction* [num()];
          bool in_scalar_section = true;

          for (size_t n = 0; maps[n].name; ++n) {
            QAction* action = new QAction (maps[n].name, parent);
            action->setCheckable (true);
            group->addAction (action);

            if (maps[n].special && in_scalar_section) {
              menu->addSeparator();
              in_scalar_section = false;
            }

            menu->addAction (action);
            parent->addAction (action);

            if (create_shortcuts)
              action->setShortcut (QObject::tr (std::string ("Ctrl+" + str (n+1)).c_str()));

            actions[n] = action;
          }

          actions[0]->setChecked (true);
        }








        Renderer::~Renderer () 
        {
          if (VB)
            glDeleteBuffers (1, &VB);
          if (VAO)
            glDeleteVertexArrays (1, &VAO);
        }





        void Renderer::setup (size_t index) 
        {
          program.clear();
          frame_program.clear();

          GL::Shader::Vertex vertex_shader (
              "layout(location=0) in vec3 data;\n"
              "uniform float scale_x, scale_y;\n"
              "out float amplitude;\n"
              "void main () {\n"
              "  gl_Position = vec4 (data.x*scale_x-1.0, data.y*scale_y-1.0, 0.0, 1.0);\n"
              "  amplitude = data.z;\n"
              "}\n");

          GL::Shader::Fragment fragment_shader (
              "in float amplitude;\n"
              "out vec3 color;\n"
              "void main () {\n"
              "  " + std::string(maps[index].mapping) + 
              "}\n");

          GL::Shader::Fragment frame_fragment_shader (
              "out vec3 color;\n"
              "void main () {\n"
              "  color = vec3(1.0, 1.0, 0.0);\n"
              "}\n");

          program.attach (vertex_shader);
          program.attach (fragment_shader);
          program.link();

          frame_program.attach (vertex_shader);
          frame_program.attach (frame_fragment_shader);
          frame_program.link();
          
          current_index = index;
        }












        void Renderer::render (const Projection& projection, const Displayable& object, int position)
        {
          if (!position) return;
          if (maps[object.colourmap()].special) return;
          
          if (!program || !frame_program || object.colourmap() != current_index)
            setup (object.colourmap());

          if (!VB || !VAO) {
            glGenBuffers (1, &VB);
            glGenVertexArrays (1, &VAO);

            glBindBuffer (GL_ARRAY_BUFFER, VB);
            glBindVertexArray (VAO);

            glEnableVertexAttribArray (0);
            glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
          }
          else {
            glBindBuffer (GL_ARRAY_BUFFER, VB);
            glBindVertexArray (VAO);
          }


          GLfloat data[] = {
            0.0f,   0.0f,  0.0f,
            0.0f,   height, 1.0f,
            width, height, 1.0f,
            width, 0.0f,  0.0f
          };
          float x_offset = offset;
          float y_offset = offset;
          int halign = -1;
          if (position == 2 || position == 4) {
            x_offset = projection.width() - width - offset;
            halign = 1;
          }
          if (position == 3 || position == 4) 
            y_offset = projection.height() - height - offset;

          data[0] += x_offset; data[1] += y_offset;
          data[3] += x_offset; data[4] += y_offset;
          data[6] += x_offset; data[7] += y_offset;
          data[9] += x_offset; data[10] += y_offset;

          glBufferData (GL_ARRAY_BUFFER, sizeof(data), data, GL_STREAM_DRAW);

          glDepthMask (GL_FALSE);
          glLineWidth (1.0);
          glDisable (GL_BLEND);
          glDisable (GL_DEPTH_TEST);

          program.start();
          glUniform1f (glGetUniformLocation (program, "scale_x"), 2.0f / projection.width());
          glUniform1f (glGetUniformLocation (program, "scale_y"), 2.0f / projection.height());
          glDrawArrays (GL_QUADS, 0, 4);
          program.stop();

          frame_program.start();
          glUniform1f (glGetUniformLocation (program, "scale_x"), 2.0f / projection.width());
          glUniform1f (glGetUniformLocation (program, "scale_y"), 2.0f / projection.height());
          glDrawArrays (GL_LINE_LOOP, 0, 4);
          frame_program.stop();

          projection.setup_render_text();
          int x = halign > 0 ? data[0] - text_offset : data[6] + text_offset;
          projection.render_text_align (x, data[1], str(object.scaling_min()), halign, 0);
          projection.render_text_align (x, data[4], str(object.scaling_max()), halign, 0);
          projection.done_render_text();

          glDepthMask (GL_TRUE);

        }



      }
    }
  }
}

