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

