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


#include <opengl/gl.h>

namespace MR {
  namespace GL {

    void init () 
    {
      static bool initialised = false;
      if (initialised) return;
      info ("GL renderer:  " + std::string ((const char*) glGetString (GL_RENDERER)));
      info ("GL version:   " + std::string ((const char*) glGetString (GL_VERSION)));
      info ("GL vendor:    " + std::string ((const char*) glGetString (GL_VENDOR)));
      info ("GL extensions:\n" + std::string ((const char*) glGetString (GL_EXTENSIONS)));
      initialised = true;
    }




    bool check_extension (const std::string& extension_name) {
      const char* ext = (const char*) glGetString (GL_EXTENSIONS);
      while (*ext) {
        while (isspace (*ext)) ++ext;
        const char* end = ext;
        while (*end && !isspace(*end)) ++end;
        if (size_t(end-ext) == extension_name.size()) 
          if (extension_name.compare (0, end-ext, ext, end-ext) == 0) return (true);
        ext = end;
      }
      return (false);
    }

  }
}

