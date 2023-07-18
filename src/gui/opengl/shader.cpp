/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
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

