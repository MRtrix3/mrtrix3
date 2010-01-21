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


#include "opengl/gl.h"

namespace MR {
  namespace GL {

    void init (const char** extensions) 
    {
      static bool initialised = false;
      if (initialised) return;
      info ("GL renderer:  " + std::string ((const char*) glGetString (GL_RENDERER)));
      info ("GL version:   " + std::string ((const char*) glGetString (GL_VERSION)));
      info ("GL vendor:    " + std::string ((const char*) glGetString (GL_VENDOR)));
      info ("GL extensions:\n" + std::string ((const char*) glGetString (GL_EXTENSIONS)));

      size_t num = 0;
      while (extensions[num]) ++num;
      bool found[num];
      for (size_t n = 0; n < num; ++n) found[n] = false;

      std::vector<std::string> ext (split ((const char*) glGetString (GL_EXTENSIONS)));
      for (std::vector<std::string>::const_iterator i = ext.begin(); i != ext.end(); ++i) 
        for (size_t n = 0; n < num; ++n) 
          if (*i == extensions[n]) found[n] = true;

      for (size_t n = 0; n < num; ++n) {
        if (!found[n]) error (std::string ("no support for required OpenGL extension \"") + extensions[n] + "\"");
        else info (std::string ("required OpenGL extension \"") + extensions[n] + "\" is supported");
      }

      for (size_t n = 0; n < num; ++n) 
        if (!found[n]) 
          throw Exception ("OpenGL environment is inadequate - aborting");

      initialised = true;
    }


  }
}

