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

#include "gui/opengl/shader.h"

namespace MR
{
  namespace GUI
  {
    namespace GL
    {
      namespace Shader
      {

        void print_log (bool is_program, const std::string& type_name, GLuint index)
        {
          int length = 0;
          int chars = 0;
          char* log;

          if (is_program) 
            gl::GetProgramiv (index, gl::INFO_LOG_LENGTH, &length);
          else 
            gl::GetShaderiv (index, gl::INFO_LOG_LENGTH, &length);

          if (length > 0) {
            log = new char [length];
            if (is_program)
              gl::GetProgramInfoLog (index, length, &chars, log);
            else 
              gl::GetShaderInfoLog (index, length, &chars, log);

            if (strlen (log)) 
              MR::print ("GLSL log [" + type_name + "]: " + log + "\n");

            delete [] log;
          }
        }

      }
    }
  }
}

