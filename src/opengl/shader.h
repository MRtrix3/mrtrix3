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

*/

#ifndef __opengl_shader_h__
#define __opengl_gl_h__

#include "opengl/gl.h"

namespace MR {
  namespace GL {
    namespace Shader {

      void init (); 
      void print_log (const std::string& type, GLhandleARB obj);

    }
  }
}

#define __DECLARE__
#include "opengl/extensions/shader.h"
#undef __DECLARE__

#endif

