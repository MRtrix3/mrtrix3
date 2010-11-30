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

#include "mrview/shader.h"

namespace MR {
  namespace Viewer {

    void Shader::set (uint32_t flags) 
    {
      if (!vertex_shader) 
        vertex_shader.compile (vertex_shader_source);

      if (shader_program) 
        shader_program.detach (fragment_shader);
      else 
        shader_program.attach (vertex_shader);

      std::string source =
        "uniform float offset, scale, lower, upper;"
        "uniform sampler";
      source += flags & Texture3D ? "3" : "2";
      source += "D tex; void main() {"
        "if (gl_TexCoord[0].s < 0.0 || gl_TexCoord[0].s > 1.0 ||"
        "    gl_TexCoord[0].t < 0.0 || gl_TexCoord[0].t > 1.0";
      if (flags & Texture3D) 
        source += " || gl_TexCoord[0].p < 0.0 || gl_TexCoord[0].p > 1.0";
      source +=  ") discard;"
        "vec4 color = texture";
      source += flags & Texture3D ? "3" : "2";
      source += "D (tex,gl_TexCoord[0].st";
      if (flags & Texture3D) source += "p";
      source += ");";

      if (flags & DiscardLower)
        source += "if (color.r < lower) discard;";

      if (flags & DiscardUpper)
        source += "if (color.r > upper) discard;";

      uint32_t colourmap = flags & ColourMap::Mask;
      if (colourmap & ColourMap::MaskNonScalar) {
        if (colourmap == ColourMap::RGB)
          source += "gl_FragColor.rgb = scale * (abs(color.rgb) - offset);";
        else if (colourmap == ColourMap::Complex) {
          assert (0);
        }
        else assert (0);
      }
      else { // scalar colourmaps:
        source += "color.rgb = scale * (color.rgb - offset);";
        if (colourmap == ColourMap::Gray)
          source += "gl_FragColor.rgb = color.rgb;";
        else if (colourmap == ColourMap::Hot)
          source += 
            "gl_FragColor.r = 2.7213 * color.r;"
            "gl_FragColor.g = 2.7213 * color.r - 1.0;"
            "gl_FragColor.b = 3.7727 * color.r - 2.7727;";
        else if (colourmap == ColourMap::Cool)
          source += 
            "gl_FragColor.r = 1.0 - 2.7213 * color.r;"
            "gl_FragColor.g = 2.0 - 2.7213 * color.r;"
            "gl_FragColor.b = 3.7727 - 3.7727 * color.r;";
        else if (colourmap == ColourMap::Jet)
          source += 
            "gl_FragColor.rgb = 1.5 - abs (color.rgb - vec3(0.25, 0.5, 0.75));";
        else assert (0);
      }

      source += 
        "gl_FragColor.a = color.a;"
        "}";

      fragment_shader.compile (source.c_str());
      shader_program.attach (fragment_shader);

      shader_program.link();
    }


    const char* Shader::vertex_shader_source = 
      "void main() {"
      "  gl_TexCoord[0] = gl_MultiTexCoord0;"
      "  gl_Position = ftransform();"
      "}";

  }
}




