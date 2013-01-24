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

      inline std::string amplitude (uint32_t flags) 
      {
        uint32_t colourmap = flags & ColourMap::Mask;
        if (colourmap == ColourMap::RGB) return "length (color.rgb)";
        if (colourmap == ColourMap::Complex) return "length (color.ra)";
        return "color.a";
      }

      void Shader::recompile ()
      {
        if (shader_program) {
          shader_program.detach (fragment_shader);
          shader_program.detach (vertex_shader);
        } 

        // vertex shader:
        std::string source = 
          "#version 330 core\n"
          "layout(location = 0) in vec3 vertpos;\n"
          "layout(location = 1) in vec3 texpos;\n"
          "uniform mat4 MVP;\n"
          "out vec3 texcoord;\n"
          "void main() {\n"
          "  gl_Position =  MVP * vec4 (vertpos,1);\n"
          "  texcoord = texpos;\n"
          "}\n";


        vertex_shader.compile (source);
        shader_program.attach (vertex_shader);


        // fragment shader:
        source = 
          "#version 330 core\n"
          "uniform float offset, scale";
        if (flags_ & DiscardLower)
          source += ", lower";
        if (flags_ & DiscardUpper)
          source += ", upper";
        if (flags_ & Transparency)
          source += ", alpha_scale, alpha_offset, alpha";

        source += 
          ";\nuniform sampler3D tex;\n"
          "in vec3 texcoord;\n"
          "out vec4 color;\n";
        if (flags_ & Lighting) 
          source += 
            "uniform float ambient;\n"
            "uniform vec3 lightDir;\n";

        source += 
          "void main() {\n"
          "  if (texcoord.s < 0.0 || texcoord.s > 1.0 ||\n"
          "      texcoord.t < 0.0 || texcoord.t > 1.0 ||\n"
          "      texcoord.p < 0.0 || texcoord.p > 1.0) discard;\n"
          "  color = texture (tex, texcoord.stp);\n"
          "  color.a = " + amplitude (flags_) + ";\n"
          "  if (isnan(color.a) || isinf(color.a)) discard;\n";
/*
        if (flags_ & Lighting) 
          source += 
            "vec4 tmp = color; vec3 normal; "
            "color = texture3D (tex, gl_TexCoord[0].stp+vec3(-2.0e-2,0.0,0.0)); normal.x = " + amplitude (flags_) + "; "
            "color = texture3D (tex, gl_TexCoord[0].stp+vec3(2.0e-2,0.0,0.0)); normal.x -= " + amplitude (flags_) + "; "
            "color = texture3D (tex, gl_TexCoord[0].stp+vec3(0.0,-2.0e-2,0.0)); normal.y = " + amplitude (flags_) + "; "
            "color = texture3D (tex, gl_TexCoord[0].stp+vec3(0.0,2.0e-2,0.0)); normal.y -= " + amplitude (flags_) + "; "
            "color = texture3D (tex, gl_TexCoord[0].stp+vec3(0.0,0.0,-2.0e-2)); normal.z = " + amplitude (flags_) + "; "
            "color = texture3D (tex, gl_TexCoord[0].stp+vec3(0.0,0.0,2.0e-2)); normal.z -= " + amplitude (flags_) + "; "
            "normal = normalize (gl_NormalMatrix * normal); "
            "color = tmp; ";
*/
        if (flags_ & DiscardLower)
          source += "if (color.a < lower) discard;";

        if (flags_ & DiscardUpper)
          source += "if (color.a > upper) discard;";

        if (flags_ & Transparency) 
          source += "if (color.a < alpha_offset) discard; "
            "float alpha = clamp ((color.a - alpha_offset) * alpha_scale, 0, alpha); ";

        uint32_t colourmap = flags_ & ColourMap::Mask;
        if (colourmap & ColourMap::MaskNonScalar) {
          if (colourmap == ColourMap::RGB)
            source += "color.rgb = scale * (abs(color.rgb) - offset);\n";
          else if (colourmap == ColourMap::Complex) {
            source += 
              "float mag = clamp (scale * (color.a - offset), 0.0, 1.0);\n"
              "float phase = atan (color.a, color.g) / 2.094395102393195;\n"
              "color.g = mag * (abs (phase));\n"
              "phase += 1.0; if (phase > 1.5) phase -= 3.0;\n"
              "color.r = mag * (abs (phase));\n"
              "phase += 1.0; if (phase > 1.5) phase -= 3.0;\n"
              "color.b = mag * (abs (phase));\n";
          }
          else assert (0);
        }
        else { // scalar colourmaps:

          source += "color.a = clamp (";
          if (flags_ & InvertScale) source += "1.0 -";
          source += " scale * (color.a - offset), 0.0, 1.0);\n";

          if (colourmap == ColourMap::Gray)
            source += "color.rgb = vec3(color.a);\n";
          else if (colourmap == ColourMap::Hot)
            source +=
              "color.r = 2.7213 * color.a;\n"
              "color.g = 2.7213 * color.a - 1.0;\n"
              "color.b = 3.7727 * color.a - 2.7727;\n";
          else if (colourmap == ColourMap::Jet)
            source +=
              "color.rgb = 1.5 - 4.0 * abs (color.a - vec3(0.25, 0.5, 0.75));\n";
          else assert (0);
        }

        if (flags_ & InvertMap)
          source += "color.rgb = 1.0 - color.rgb;";
/*
        // TODO: lighting code in here:
        if (flags_ & Lighting) 
          source += 
            "  vec3 halfV;\n"
            "  float NdotL, NdotHV;\n"
            "  NdotL = dot (normal, lightDir);\n"
            "  color.rgb = gl_FragColor.rgb * ambient.rgb;\n"
            "  color.rgb += gl_LightSource[0].diffuse.rgb * NdotL * gl_FragColor.rgb;\n"
            "  halfV = normalize (halfVector);\n"
            "  NdotHV = max (dot (normal, halfV), 0.0);\n"
            "  gl_FragColor.rgb = color.rgb + gl_FrontMaterial.specular.rgb * gl_LightSource[0].specular.rgb * pow (NdotHV, gl_FrontMaterial.shininess);\n";
*/
        if (flags_ & Transparency) 
          source += "color.a = alpha;\n";
        source += "}\n";

        fragment_shader.compile (source.c_str());
        shader_program.attach (fragment_shader);

        shader_program.link();
      }



    }
  }
}




