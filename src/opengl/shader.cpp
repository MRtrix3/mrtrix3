/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 12/01/09.

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


    24-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * added support of overlay of orientation plot on main window

*/

#include "opengl/shader.h"

namespace MR {
  namespace GL {
    namespace Shader {

      bool supported () {
        static bool retval = GLEW_ARB_vertex_shader && GLEW_ARB_fragment_shader;
        static bool warning_isued = false;
        if (!warning_isued) {
          warning_isued = true;
          if (!retval) error ("WARNING: vertex shading ARB extension is NOT supported - advanced features will be disabled");
          else info ("vertex shading ARB extension is supported");
        }
        return (retval);
      }

      void print_log (const std::string& type, GLhandleARB obj)
      {
        int length = 0;
        int chars = 0;
        char *log;

        glGetObjectParameterivARB (obj, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
        if (length > 0) {
          log = new char [length];
          glGetInfoLogARB (obj, length, &chars, log);
          if (strlen(log)) MR::print ("GLSL log [" + type + "]: " + log + "\n");
          delete [] log;
        }
      }

    }
  }
}



