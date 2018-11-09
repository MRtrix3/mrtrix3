/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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
              FAIL ("GLSL log [" + type_name + "]: " + log);

            delete [] log;
          }
        }

      }
    }
  }
}

