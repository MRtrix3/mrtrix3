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

    namespace
    {
      class OrientationLabel
      {
        public:
          OrientationLabel () { }
          OrientationLabel (const Point<>& direction, const char textlabel) :
            dir (direction), label (1, textlabel) { }
          Point<> dir;
          std::string label;
          bool operator< (const OrientationLabel& R) const {
            return dir.norm2() < R.dir.norm2();
          }
      };
    }


    void Projection::render_crosshairs (const Point<>& focus) const
    {
      if (!crosshairs_VB || !crosshairs_VAO) {
        crosshairs_VB.gen();
        crosshairs_VAO.gen();

        crosshairs_VB.bind (GL_ARRAY_BUFFER);
        crosshairs_VAO.bind();

        glEnableVertexAttribArray (0);
        glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
      }
      else {
        crosshairs_VB.bind (GL_ARRAY_BUFFER);
        crosshairs_VAO.bind();
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
            "  color = vec4 (0.5, 0.5, 0.0, 1.0);\n"
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

      glDepthMask (GL_TRUE);
      glDisable (GL_BLEND);
      glLineWidth (1.0);

      crosshairs_program.start();
      glDrawArrays (GL_LINES, 0, 4);
      crosshairs_program.stop();
    }




    void Projection::draw_orientation_labels () const
    {
      std::vector<OrientationLabel> labels;
      labels.push_back (OrientationLabel (model_to_screen_direction (Point<> (-1.0, 0.0, 0.0)), 'L'));
      labels.push_back (OrientationLabel (model_to_screen_direction (Point<> (1.0, 0.0, 0.0)), 'R'));
      labels.push_back (OrientationLabel (model_to_screen_direction (Point<> (0.0, -1.0, 0.0)), 'P'));
      labels.push_back (OrientationLabel (model_to_screen_direction (Point<> (0.0, 1.0, 0.0)), 'A'));
      labels.push_back (OrientationLabel (model_to_screen_direction (Point<> (0.0, 0.0, -1.0)), 'I'));
      labels.push_back (OrientationLabel (model_to_screen_direction (Point<> (0.0, 0.0, 1.0)), 'S'));

      setup_render_text (1.0, 0.0, 0.0);
      std::sort (labels.begin(), labels.end());
      for (size_t i = 2; i < labels.size(); ++i) {
        float pos[] = { labels[i].dir[0], labels[i].dir[1] };
        float dist = std::min (width()/Math::abs (pos[0]), height()/Math::abs (pos[1])) / 2.0;
        int x = Math::round (width() /2.0 + pos[0]*dist);
        int y = Math::round (height() /2.0 + pos[1]*dist);
        render_text_inset (x, y, std::string (labels[i].label));
      }
      done_render_text();
    }





  }
}

