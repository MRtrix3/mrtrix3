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

#include "debug.h"
#include "gui/mrview/shader.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      void Shader::set (uint32_t flags)
      {
        if (!vertex_shader)
          vertex_shader.compile (vertex_shader_source);

        if (shader_program)
          shader_program.detach (fragment_shader);
        else
          shader_program.attach (vertex_shader);

        std::string source =
          "uniform float offset, scale";
        if (flags & DiscardLower)
          source += ", lower";
        if (flags & DiscardUpper)
          source += ", upper";

        source += "; uniform sampler3D tex; void main() {"
                  "if (gl_TexCoord[0].s < 0.0 || gl_TexCoord[0].s > 1.0 ||"
                  "    gl_TexCoord[0].t < 0.0 || gl_TexCoord[0].t > 1.0 ||"
                  "    gl_TexCoord[0].p < 0.0 || gl_TexCoord[0].p > 1.0) discard;"
                  "vec4 color = texture3D (tex,gl_TexCoord[0].stp);";

        uint32_t colourmap = flags & ColourMap::Mask;
        if (colourmap & ColourMap::MaskNonScalar) {
          if (colourmap == ColourMap::RGB)
            source += "gl_FragColor.rgb = scale * (abs(color.rgb) - offset);";
          else if (colourmap == ColourMap::Complex) {
            source += 
              "float mag = clamp (scale * (sqrt (color.r*color.r + color.a*color.a) - offset), 0.0, 1.0); "
              "float phase = atan (color.a, color.g) / 2.094395102393195; "
              "color.g = mag * (abs (phase)); "
              "phase += 1.0; if (phase > 1.5) phase -= 3.0; "
              "color.r = mag * (abs (phase)); "
              "phase += 1.0; if (phase > 1.5) phase -= 3.0; "
              "color.b = mag * (abs (phase)); "
              "gl_FragColor.rgb = color.rgb; "
              "color.a = mag;";
          }
          else assert (0);
        }
        else { // scalar colourmaps:

          if (flags & DiscardLower)
            source += "if (color.r < lower) discard;";

          if (flags & DiscardUpper)
            source += "if (color.r > upper) discard;";

          source += "color.rgb = clamp (";
          if (flags & InvertScale) source += "1.0 -";
          source += " scale * (color.rgb - offset), 0.0, 1.0);";
          if (colourmap == ColourMap::Gray)
            source += "gl_FragColor.rgb = color.rgb;";
          else if (colourmap == ColourMap::Hot)
            source +=
              "color.r = clamp (color.r, 0.0, 1.0);"
              "gl_FragColor.r = 2.7213 * color.r;"
              "gl_FragColor.g = 2.7213 * color.r - 1.0;"
              "gl_FragColor.b = 3.7727 * color.r - 2.7727;";
          else if (colourmap == ColourMap::Jet)
            source +=
              "gl_FragColor.rgb = 1.5 - 4.0 * abs (color.rgb - vec3(0.25, 0.5, 0.75));";
          else assert (0);
        }

        if (flags & InvertMap)
          source += "gl_FragColor = 1.0 - gl_FragColor;";
        source +=
          "gl_FragColor.a = color.a;"
          "}";

        fragment_shader.compile (source.c_str());
        shader_program.attach (fragment_shader);

        shader_program.link();
      }

      GL::Shader::Vertex Shader::vertex_shader;

      const char* Shader::vertex_shader_source =
        "void main() {"
        "  gl_TexCoord[0] = gl_MultiTexCoord0;"
        "  gl_Position = ftransform();"
        "}";

    }
  }
}




