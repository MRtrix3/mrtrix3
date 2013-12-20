/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#include "math/vector.h"
#include "gui/mrview/mode/slice.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {


        std::string Slice::Shader::vertex_shader_source (const Displayable& object) 
        {
          return
            "layout(location = 0) in vec3 vertpos;\n"
            "layout(location = 1) in vec3 texpos;\n"
            "uniform mat4 MVP;\n"
            "out vec3 texcoord;\n"
            "void main() {\n"
            "  gl_Position =  MVP * vec4 (vertpos,1);\n"
            "  texcoord = texpos;\n"
            "}\n";
        }



        std::string Slice::Shader::fragment_shader_source (const Displayable& object)
        {
          std::string source = object.declare_shader_variables () +
            "uniform sampler3D tex;\n"
            "in vec3 texcoord;\n"
            "out vec4 color;\n";

          source +=
            "void main() {\n"
            "  if (texcoord.s < 0.0 || texcoord.s > 1.0 ||\n"
            "      texcoord.t < 0.0 || texcoord.t > 1.0 ||\n"
            "      texcoord.p < 0.0 || texcoord.p > 1.0) discard;\n"
            "  color = texture (tex, texcoord.stp);\n"
            "  float amplitude = " + std::string (ColourMap::maps[object.colourmap].amplitude) + ";\n"
            "  if (isnan(amplitude) || isinf(amplitude)) discard;\n";

          if (object.use_discard_lower())
            source += "if (amplitude < lower) discard;\n";

          if (object.use_discard_upper())
            source += "if (amplitude > upper) discard;\n";

          if (object.use_transparency())
            source += "if (amplitude < alpha_offset) discard;\n"
              "float alpha = clamp ((amplitude - alpha_offset) * alpha_scale, 0, alpha);\n";

          if (!ColourMap::maps[object.colourmap].special) {
            source += "  amplitude = clamp (";
            if (object.scale_inverted())
              source += "1.0 -";
            source += " scale * (amplitude - offset), 0.0, 1.0);\n  ";
          }

          source += ColourMap::maps[object.colourmap].mapping;

          if (object.use_transparency())
            source += "color.a = alpha;\n";
          source += "}\n";

          return source;
        }








        Slice::~Slice () { }


        void Slice::paint (Projection& with_projection)
        {
          // set up OpenGL environment:
          glDisable (GL_BLEND);
          glEnable (GL_TEXTURE_3D);
          glDisable (GL_DEPTH_TEST);
          glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDepthMask (GL_FALSE);
          glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

          draw_plane (plane(), slice_shader, with_projection);
        }


        void Slice::draw_plane (int axis, Displayable::Shader& shader_program, Projection& with_projection)
        {
          // info for projection:
          float fov = FOV() / (float) (with_projection.width()+with_projection.height());
          float depth = std::max (std::max (image()->header().dim(0), image()->header().dim(1)), image()->header().dim(2));

          // set up projection & modelview matrices:
          GL::mat4 P = GL::ortho (
              -with_projection.width()*fov, with_projection.width()*fov,
              -with_projection.height()*fov, with_projection.height()*fov,
              -depth, depth);
          GL::mat4 M = snap_to_image() ? GL::mat4 (image()->interp.image2scanner_matrix()) : GL::mat4 (orientation());
          GL::mat4 MV = adjust_projection_matrix (M, axis) * GL::translate (-target());
          with_projection.set (MV, P);

          // render image:
          DEBUG_OPENGL;
          if (snap_to_image())
            image()->render2D (shader_program, with_projection, axis, slice(axis));
          else
            image()->render3D (shader_program, with_projection, with_projection.depth_of (focus()));
          DEBUG_OPENGL;

          glDisable (GL_TEXTURE_3D);

          render_tools (with_projection);

          draw_crosshairs (with_projection);
          draw_orientation_labels (with_projection);
        }






      }
    }
  }
}


