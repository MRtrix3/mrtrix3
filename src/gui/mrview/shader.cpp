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
        if (colourmap == ColourMap::Complex) return "sqrt (color.r*color.r + color.a*color.a)";
        return "color.a";
      }

      void Shader::recompile ()
      {
        if (shader_program) {
          shader_program.detach (fragment_shader);
          shader_program.detach (vertex_shader);
        } 

        // vertex shader:
        std::string source;
        if (Lighting) 
          source += "varying vec4 ambient; "
            "varying vec3 lightDir, halfVector; ";
        source += "void main() { ";
        if (Lighting) 
            source +=
              "lightDir = normalize (vec3 (gl_LightSource[0].position)); "
              "halfVector = normalize (gl_LightSource[0].halfVector.xyz); "
              "ambient = gl_LightSource[0].ambient + gl_LightModel.ambient; ";
        source += 
          "gl_TexCoord[0] = gl_MultiTexCoord0; "
          "gl_Position = ftransform(); "
          "}";


        vertex_shader.compile (source);
        shader_program.attach (vertex_shader);


        // fragment shader:
        source = "uniform float offset, scale";
        if (flags_ & DiscardLower)
          source += ", lower";
        if (flags_ & DiscardUpper)
          source += ", upper";
        if (flags_ & Transparency)
          source += ", alpha_scale, alpha_offset, alpha";

        source += "; uniform sampler3D tex; ";
        if (flags_ & Lighting) 
          source += 
            "varying vec4 ambient; "
            "varying vec3 lightDir, halfVector; ";

        source += 
          "void main() {"
          "if (gl_TexCoord[0].s < 0.0 || gl_TexCoord[0].s > 1.0 ||"
          "    gl_TexCoord[0].t < 0.0 || gl_TexCoord[0].t > 1.0 ||"
          "    gl_TexCoord[0].p < 0.0 || gl_TexCoord[0].p > 1.0) discard; "
          " vec4 color; ";


        if (flags_ & Lighting) 
          source += 
            "vec3 normal; "
            "color = texture3D (tex, gl_TexCoord[0].stp+vec3(2.0e-2,0.0,0.0)); normal.x = " + amplitude (flags_) + "; "
            "color = texture3D (tex, gl_TexCoord[0].stp+vec3(-2.0e-2,0.0,0.0)); normal.x -= " + amplitude (flags_) + "; "
            "color = texture3D (tex, gl_TexCoord[0].stp+vec3(0.0,2.0e-2,0.0)); normal.y = " + amplitude (flags_) + "; "
            "color = texture3D (tex, gl_TexCoord[0].stp+vec3(0.0,-2.0e-2,0.0)); normal.y -= " + amplitude (flags_) + "; "
            "color = texture3D (tex, gl_TexCoord[0].stp+vec3(0.0,0.0,2.0e-2)); normal.z = " + amplitude (flags_) + "; "
            "color = texture3D (tex, gl_TexCoord[0].stp+vec3(0.0,0.0,-2.0e-2)); normal.z -= " + amplitude (flags_) + "; "
            "normal = normalize (gl_NormalMatrix * normal); ";

        source +=
          "color = texture3D (tex,gl_TexCoord[0].stp); "
          "gl_FragColor.a = " + amplitude (flags_) + "; ";

        if (flags_ & DiscardLower)
          source += "if (gl_FragColor.a < lower) discard;";

        if (flags_ & DiscardUpper)
          source += "if (gl_FragColor.a > upper) discard;";

        if (flags_ & Transparency) 
          source += "if (gl_FragColor.a < alpha_offset) discard; "
            "gl_FragColor.a = clamp ((gl_FragColor.a - alpha_offset) * alpha_scale, 0, alpha); ";

        uint32_t colourmap = flags_ & ColourMap::Mask;
        if (colourmap & ColourMap::MaskNonScalar) {
          if (colourmap == ColourMap::RGB)
            source += "gl_FragColor.rgb = scale * (abs(color.rgb) - offset);";
          else if (colourmap == ColourMap::Complex) {
            source += 
              "float mag = clamp (scale * (gl_FragColor.a - offset), 0.0, 1.0); "
              "float phase = atan (color.a, color.g) / 2.094395102393195; "
              "color.g = mag * (abs (phase)); "
              "phase += 1.0; if (phase > 1.5) phase -= 3.0; "
              "color.r = mag * (abs (phase)); "
              "phase += 1.0; if (phase > 1.5) phase -= 3.0; "
              "color.b = mag * (abs (phase)); "
              "gl_FragColor.rgb = color.rgb; ";
          }
          else assert (0);
        }
        else { // scalar colourmaps:

          source += "color.a = clamp (";
          if (flags_ & InvertScale) source += "1.0 -";
          source += " scale * (color.a - offset), 0.0, 1.0);";

          if (colourmap == ColourMap::Gray)
            source += "gl_FragColor.rgb = color.a;";
          else if (colourmap == ColourMap::Hot)
            source +=
              "color.r = clamp (color.a, 0.0, 1.0);"
              "gl_FragColor.r = 2.7213 * color.a;"
              "gl_FragColor.g = 2.7213 * color.a - 1.0;"
              "gl_FragColor.b = 3.7727 * color.a - 2.7727;";
          else if (colourmap == ColourMap::Jet)
            source +=
              "gl_FragColor.rgb = 1.5 - 4.0 * abs (color.a - vec3(0.25, 0.5, 0.75));";
          else assert (0);
        }

        if (flags_ & InvertMap)
          source += "gl_FragColor.rgb = 1.0 - gl_FragColor.rgb;";

        // TODO: lighting code in here:
        if (flags_ & Lighting) 
          source += 
            "  vec3 halfV; "
            "  float NdotL, NdotHV; "
            "  NdotL = dot (normal, lightDir); "
            "  color.rgb = gl_FragColor.rgb * ambient.rgb; "
            "  color.rgb += gl_LightSource[0].diffuse.rgb * NdotL * gl_FragColor.rgb; "
            "  halfV = normalize (halfVector); "
            "  NdotHV = max (dot (normal, halfV), 0.0); "
            "  gl_FragColor.rgb = color.rgb + gl_FrontMaterial.specular.rgb * gl_LightSource[0].specular.rgb * pow (NdotHV, gl_FrontMaterial.shininess); ";

        source += "}";

        fragment_shader.compile (source.c_str());
        shader_program.attach (fragment_shader);

        shader_program.link();
      }



    }
  }
}




