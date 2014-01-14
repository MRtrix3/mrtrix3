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
        GLint gl_version, gl_version_major;
        glGetIntegerv (GL_MAJOR_VERSION, &gl_version_major);
        glGetIntegerv (GL_MINOR_VERSION, &gl_version);
        gl_version += 10*gl_version_major;
        if (gl_version < 33)
          FAIL ("your OpenGL implementation is not sufficient to run MRView - need version 3.3 or higher");
/*
        GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);
        switch (status) {
          case GL_FRAMEBUFFER_UNDEFINED: throw Exception ("default framebuffer does not exist!");
          case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: throw Exception ("default framebuffer is incomplete!");
          case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: throw Exception ("default framebuffer has no image attached!");
          case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: throw Exception ("default framebuffer has incomplete draw buffer!");
          case GL_FRAMEBUFFER_UNSUPPORTED: throw Exception ("default framebuffer does not support requested format!");
          case GL_FRAMEBUFFER_COMPLETE: break;
          default: throw Exception ("default framebuffer cannot be used for unknown reason!");
        }
        */
      }

      const char* ErrorString (GLenum errorcode) 
      {
        switch (errorcode) {
          case GL_INVALID_ENUM: return "invalid value for enumerated argument";
          case GL_INVALID_VALUE: return "value out of range";
          case GL_INVALID_OPERATION: return "operation not allowed given current state";
          case GL_OUT_OF_MEMORY: return "insufficient memory";
          case GL_INVALID_FRAMEBUFFER_OPERATION: return "invalid framebuffer operation";
          default: return "unknown error";
        }
      }

    }
  }
}



