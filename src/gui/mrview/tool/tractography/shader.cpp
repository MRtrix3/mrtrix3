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

          if (shader_program) 
            shader_program.clear();

          std::string vertex_shader_code =
              "#version 330 core \n "
              "layout(location = 0) in vec3 vertexPosition_modelspace;\n"
              "layout(location = 1) in vec3 previousVertex;\n "
              "layout(location = 2) in vec3 nextVertex;\n "
              "out vec3 fragmentColor;\n"
              "uniform mat4 MVP;\n";

          if (do_crop_to_slab) {
            vertex_shader_code +=
                "out float include;\n"
                "uniform vec3 screen_normal;\n"
                "uniform float crop_var;\n"
                "uniform float slab_width;\n";
          }

          vertex_shader_code +=
              "void main() {\n"
              "  gl_Position =  MVP * vec4(vertexPosition_modelspace,1);\n"
              "  if (isnan(previousVertex.x))\n"
              "    fragmentColor = nextVertex - vertexPosition_modelspace;\n"
              "  else if (isnan(nextVertex.x))\n"
              "    fragmentColor = vertexPosition_modelspace - previousVertex;\n"
              "  else\n"
              "    fragmentColor = nextVertex - previousVertex;\n"
              "  fragmentColor = normalize (abs(fragmentColor));\n";

          if (do_crop_to_slab) {
            vertex_shader_code +=
                "  include = (dot(vertexPosition_modelspace, screen_normal) - crop_var) / slab_width;\n";
          }
          vertex_shader_code += "}\n";



          std::string fragment_shader_code =
              "#version 330 core\n"
              "in vec3 fragmentColor;\n"
              "in float include; \n"
              "out vec3 color;\n"
              "void main(){\n";
          if (do_crop_to_slab) {
            fragment_shader_code +=
                "  if (include < 0 || include > 1) \n"
                "    discard; \n";
          }

          fragment_shader_code +=
              " color = normalize(fragmentColor);\n"
              "}\n";



          GL::Shader::Vertex vertex_shader (vertex_shader_code);
          GL::Shader::Fragment fragment_shader (fragment_shader_code);
          shader_program.attach (vertex_shader);
          shader_program.attach (fragment_shader);
          shader_program.link();

        }

      }
    }
  }
}

