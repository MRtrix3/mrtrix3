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

#ifndef __gui_opengl_font_h__
#define __gui_opengl_font_h__

#include <QtGui>

#include "gui/opengl/shader.h"


namespace MR
{
  namespace GUI
  {
    namespace GL
    {

      class Font {
        public:
          Font (const QFont& font) :
            metric (font),
            font (font),
            tex_ID (0),
            vertex_array_object_ID (0) { } 
          ~Font ();

          void initialise ();

          const QFontMetrics metric;

        void setupGL (int width, int height, float red, float green, float blue) const {
          assert (program);
          glDisable (GL_DEPTH_TEST);
          glDepthMask (GL_FALSE);
          glEnable (GL_BLEND);
          glBlendEquation (GL_FUNC_ADD);
          glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          program.start();
          program.get_uniform ("scale_x") = float (2.0 / width);
          program.get_uniform ("scale_y") = float (2.0 / height);
          program.get_uniform ("red") = red;
          program.get_uniform ("green") = green;
          program.get_uniform ("blue") = blue;
        }

        void resetGL () const {
          program.stop();
          glDepthMask (GL_TRUE);
          glDisable (GL_BLEND);
        }

        void render (const std::string& text, int x, int y) const;

        protected:
          const QFont& font;
          GLuint tex_ID, vertex_buffer_ID[2], vertex_array_object_ID;
          GL::Shader::Program program;
          int font_width[256], font_height;
          float font_tex_pos[256], font_tex_width[256];
      };


    }
  }
}

#endif


