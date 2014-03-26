/*
   Copyright 2014 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt 2014.

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

#include "gui/mrview/tool/fixel/image.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        std::string FixelImage::Shader::vertex_shader_source (const Displayable& fixel)
        {

          std::string source =
              "layout (location = 0) in vec3 vertexPosition_modelspace;\n"
              "layout (location = 1) in vec3 previousVertex;\n"
              "layout (location = 2) in vec3 nextVertex;\n"
              "uniform mat4 MVP;\n"
              "flat out float amp_out;\n"
              "out vec3 fragmentColour;\n";

          switch (color_type) {
            case Direction: break;
            case Colour:
              source += "uniform vec3 const_colour;\n";
              break;
            case Value:
              source += "layout (location = 3) in float amp;\n"
                        "uniform float offset, scale;\n";
              break;
          }

          source +=
              "void main() {\n"
              "  gl_Position =  MVP * vec4(vertexPosition_modelspace,1);\n";

//          if (color_type == Direction)
//            source +=
//              "  vec3 dir;\n"
//              "  if (isnan (previousVertex.x))\n"
//              "    dir = nextVertex - vertexPosition_modelspace;\n"
//              "  else if (isnan (nextVertex.x))\n"
//              "    dir = vertexPosition_modelspace - previousVertex;\n"
//              "  else\n"
//              "    dir = nextVertex - previousVertex;\n";
//              source += "  fragmentColour = dir;\n";

          switch (color_type) {
            case Colour:
              source +=
                  "  fragmentColour = const_colour;\n";
              break;
            case Value:
              source += "  amp_out = amp;\n";
              if (!ColourMap::maps[colourmap].special) {
                source += "  float amplitude = clamp (";
                if (fixel.scale_inverted())
                  source += "1.0 -";
                source += " scale * (amp - offset), 0.0, 1.0);\n  ";
              }
              break;
            default:
              break;
          }

          source += "}\n";

          return source;
        }



        std::string FixelImage::Shader::fragment_shader_source (const Displayable& fixel)
        {
          std::string source =
              "in float include; \n"
              "out vec3 color;\n"
              "flat in float amp_out;\n"
              "in vec3 fragmentColour;\n";
          if (color_type == Value) {
            if (fixel.use_discard_lower())
              source += "uniform float lower;\n";
            if (fixel.use_discard_upper())
              source += "uniform float upper;\n";
          }

          source +=
              "void main(){\n";

          if (color_type == Value) {
            if (fixel.use_discard_lower())
              source += "  if (amp_out < lower) discard;\n";
            if (fixel.use_discard_upper())
              source += "  if (amp_out > upper) discard;\n";
          }

          source +=
            std::string("  color = ") + ((color_type == Direction) ? "normalize (abs (fragmentColour))" : "fragmentColour" ) + ";\n";

          source += "}\n";

          return source;
        }


        bool FixelImage::Shader::need_update (const Displayable& object) const
        {
          const FixelImage& fixel (dynamic_cast<const FixelImage&> (object));
          if (color_type != fixel.color_type)
            return true;
          return Displayable::Shader::need_update (object);
        }




        void FixelImage::Shader::update (const Displayable& object)
        {
          const FixelImage& fixel (dynamic_cast<const FixelImage&> (object));
          do_crop_to_slice = fixel.fixel_tool.do_crop_to_slice;
          color_type = fixel.color_type;
          Displayable::Shader::update (object);
        }

      }
    }
  }
}
