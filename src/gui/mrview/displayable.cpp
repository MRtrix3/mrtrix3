/*
   Copyright 2012 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier and David Raffelt, 07/12/12.

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

#include <QMenu>

#include "gui/mrview/displayable.h"
#include "progressbar.h"
#include "image/stride.h"
#include "gui/mrview/image.h"
#include "gui/mrview/window.h"
#include "gui/projection.h"



namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      Displayable::Displayable (const std::string& filename) :
        QAction (NULL),
        show (true),
        lessthan (NAN),
        greaterthan (NAN),
        display_midpoint (NAN),
        display_range (NAN),
        transparent_intensity (NAN),
        opaque_intensity (NAN),
        alpha (NAN),
        filename (filename),
        value_min (NAN),
        value_max (NAN),
        flags_ (0x00000000),
        colourmap_index (0) { }


      Displayable::Displayable (Window& window, const std::string& filename) :
        QAction (shorten (filename, 20, 0).c_str(), &window),
        show (true),
        lessthan (NAN),
        greaterthan (NAN),
        display_midpoint (NAN),
        display_range (NAN),
        transparent_intensity (NAN),
        opaque_intensity (NAN),
        alpha (NAN),
        filename (filename),
        value_min (NAN),
        value_max (NAN),
        flags_ (0x00000000),
        colourmap_index (0) {
          connect (this, SIGNAL(scalingChanged()), &window, SLOT(on_scaling_changed()));
      }


      Displayable::~Displayable ()
      {
      }

      void Displayable::recompile ()
      {
        if (shader_program)
          shader_program.clear();

        // vertex shader:
        GL::Shader::Vertex vertex_shader (
            "layout(location = 0) in vec3 vertpos;\n"
            "layout(location = 1) in vec3 texpos;\n"
            "uniform mat4 MVP;\n"
            "out vec3 texcoord;\n"
            "void main() {\n"
            "  gl_Position =  MVP * vec4 (vertpos,1);\n"
            "  texcoord = texpos;\n"
            "}\n");



        // fragment shader:
        std::string source =
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

        source +=
            "void main() {\n"
            "  if (texcoord.s < 0.0 || texcoord.s > 1.0 ||\n"
            "      texcoord.t < 0.0 || texcoord.t > 1.0 ||\n"
            "      texcoord.p < 0.0 || texcoord.p > 1.0) discard;\n"
            "  color = texture (tex, texcoord.stp);\n"
            "  float amplitude = " + std::string (ColourMap::maps[colourmap_index].amplitude) + ";\n"
            "  if (isnan(amplitude) || isinf(amplitude)) discard;\n";

        if (flags_ & DiscardLower)
          source += "if (amplitude < lower) discard;";

        if (flags_ & DiscardUpper)
          source += "if (amplitude > upper) discard;";

        if (flags_ & Transparency)
          source += "if (amplitude < alpha_offset) discard; "
              "float alpha = clamp ((amplitude - alpha_offset) * alpha_scale, 0, alpha); ";

        if (!ColourMap::maps[colourmap_index].special) {
          source += "  amplitude = clamp (";
          if (flags_ & InvertScale) source += "1.0 -";
          source += " scale * (amplitude - offset), 0.0, 1.0);\n  ";
        }

        source += ColourMap::maps[colourmap_index].mapping;

        if (flags_ & InvertMap)
          source += "color.rgb = 1.0 - color.rgb;";

        if (flags_ & Transparency)
          source += "color.a = alpha;\n";
        source += "}\n";



        GL::Shader::Fragment fragment_shader (source.c_str());

        shader_program.attach (vertex_shader);
        shader_program.attach (fragment_shader);
        shader_program.link();
      }


    }
  }
}


