/*
   Copyright 2013 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt, 15/01/13.

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

#include "gui/mrview/tool/tractography/shader.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      namespace Tool
      {

        void Shader::recompile ()
        {

          if (shader_program) {
            shader_program.detach (fragment_shader);
            shader_program.detach (vertex_shader);
          }

          std::string vertex_shader_code =
              "#version 330 core \n "
              "layout(location = 0) in vec3 vertexPosition_modelspace;\n"
              "layout(location = 1) in vec3 previousVertex;\n "
              "layout(location = 2) in vec3 nextVertex;\n "
              "out vec4 fragmentColor;\n"
              "uniform mat4 MVP;\n"
              "uniform float alpha;\n";

          if (do_crop_to_slab) {
            vertex_shader_code +=
                "out float include;\n"
                "uniform vec3 screen_normal;\n"
                "uniform float crop_var;\n"
                "uniform float slab_width;\n";
          }

          vertex_shader_code +=
              "void main() {\n"
              "  vec3 temp_colour;"
              "  gl_Position =  MVP * vec4(vertexPosition_modelspace,1);\n"
              "  if (isnan(previousVertex.x))\n"
              "    temp_colour = nextVertex - vertexPosition_modelspace;\n"
              "  else if (isnan(nextVertex.x))\n"
              "    temp_colour = vertexPosition_modelspace - previousVertex;\n"
              "  else\n"
              "    temp_colour = nextVertex - previousVertex;\n"
              "  temp_colour = normalize (abs(temp_colour));\n"
              "  fragmentColor[0] = temp_colour[0]; \n"
              "  fragmentColor[1] = temp_colour[1]; \n"
              "  fragmentColor[2] = temp_colour[2];"
              "  fragmentColor[3] = alpha; \n";

          if (do_crop_to_slab) {
            vertex_shader_code +=
                "  include = (dot(vertexPosition_modelspace, screen_normal) - crop_var) / slab_width;\n";
          }
          vertex_shader_code += "}";

          std::string fragment_shader_code =
              "#version 330 core\n"
              "in vec4 fragmentColor;\n"
              "in vec4 gl_FragCoord;"
              "in float include; \n"
              "out vec4 color;\n"
              "void main(){\n"
              "  color = fragmentColor;\n";

          if (do_crop_to_slab) {
            fragment_shader_code +=
                "  if (include < 0 || include > 1) \n"
                "    discard; \n";
          }

          fragment_shader_code +=  "}";

          vertex_shader.compile (vertex_shader_code);
          shader_program.attach (vertex_shader);
          fragment_shader.compile (fragment_shader_code);
          shader_program.attach (fragment_shader);
          shader_program.link();

        }

      }
    }
  }
}

