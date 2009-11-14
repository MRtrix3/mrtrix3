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

#ifndef __open_gl_h__
#define __open_gl_h__

#include <GL/glew.h>
#include <GL/glu.h>
#include <QtOpenGL>
#include "mrtrix.h"

namespace MR {
  namespace GL {

    void init ();

  }
}

#define DEBUG_OPENGL { GLenum error_code = glGetError(); \
  if (error_code != GL_NO_ERROR) error (MR::std::string ("OpenGL Error: ") + (const char*) gluErrorString (error_code) + " ["__FILE__":" + MR::str(__LINE__) + "]"); }

#endif

