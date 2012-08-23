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


#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace GL
    {

      void init ()
      {
        INFO ("GL renderer:  " + std::string ( (const char*) glGetString (GL_RENDERER)));
        INFO ("GL version:   " + std::string ( (const char*) glGetString (GL_VERSION)));
        INFO ("GL vendor:    " + std::string ( (const char*) glGetString (GL_VENDOR)));
        if (!GLEE_VERSION_3_0)
          throw Exception ("your OpenGL implementation is not sufficient to run MRView - need version 3.0 or higher");
        GLboolean retval;
        glGetBooleanv (GL_STEREO, &retval);
        INFO ("Stereo buffering: " + std::string ( retval ? "" : "not" ) + " supported");
      }

      const char* ErrorString (GLenum errorcode) 
      {
        switch (errorcode) {
          case GL_INVALID_ENUM: return "invalid value for enumerated argument";
          case GL_INVALID_VALUE: return "value out of range";
          case GL_INVALID_OPERATION: return "operation not allowed given current state";
          case GL_STACK_OVERFLOW: return "stack overflow";
          case GL_STACK_UNDERFLOW: return "stack underflow";
          case GL_OUT_OF_MEMORY: return "insufficient memory";
          case GL_TABLE_TOO_LARGE: return "table exceeds maximum supported size";
          default: return "unknown error";
        }
      }

    }
  }
}

