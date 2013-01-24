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

#ifndef __gui_opengl_gl_h__
#define __gui_opengl_gl_h__

#include <QtGui>

#include "glee.h"
#include "mrtrix.h"


//#define USE_OPENGL3

namespace MR
{
  namespace GUI
  {
    namespace GL
    {

      void init ();

      const char* ErrorString (GLenum errorcode);

    }
  }
}

#define DEBUG_OPENGL { GLenum error_code = glGetError(); \
    if (error_code != GL_NO_ERROR) { \
      ERROR (std::string ("OpenGL Error: ") + (const char*) MR::GUI::GL::ErrorString (error_code) + " ["__FILE__":" + MR::str(__LINE__) + "]"); \
    }\
  }

#endif

