/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "gui/mrview/mode/slice.h"

#include "gui/opengl/transformation.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {


        std::string Slice::Shader::vertex_shader_source (const Displayable&)
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
            source += "  if (amplitude < lower) discard;\n";

          if (object.use_discard_upper())
            source += "  if (amplitude > upper) discard;\n";

          if (object.use_transparency())
            source += "  if (amplitude < alpha_offset) discard;\n"
              "  color.a = clamp ((amplitude - alpha_offset) * alpha_scale, 0, alpha);\n";

          if (!ColourMap::maps[object.colourmap].special) {
            source += "  amplitude = clamp (";
            if (object.scale_inverted())
              source += "1.0 -";
            source += " scale * (amplitude - offset), 0.0, 1.0);\n  ";
          }

          source += ColourMap::maps[object.colourmap].glsl_mapping;

          source += "}\n";

          return source;
        }








        Slice::~Slice () { }


        void Slice::paint (Projection& with_projection)
        {
          GL::assert_context_is_current();
          // set up OpenGL environment:
          gl::Disable (gl::BLEND);
          gl::Disable (gl::DEPTH_TEST);
          gl::DepthMask (gl::FALSE_);
          gl::ColorMask (gl::TRUE_, gl::TRUE_, gl::TRUE_, gl::TRUE_);

          draw_plane (plane(), slice_shader, with_projection);
          GL::assert_context_is_current();
        }

        // Draw without setting up matrices/no crosshairs/no orientation labels
        void Slice::draw_plane_primitive (int axis, Displayable::Shader& shader_program, Projection& with_projection)
        {
          GL::assert_context_is_current();
          // render image:
          if (visible) {
            if (snap_to_image())
              image()->render2D (shader_program, with_projection, axis, slice (axis));
            else
              image()->render3D (shader_program, with_projection, with_projection.depth_of (focus()));
          }

          render_tools (with_projection, false, axis, slice (axis));
          GL::assert_context_is_current();
        }


        void Slice::draw_plane (int axis, Displayable::Shader& shader_program, Projection& with_projection)
        {
          GL::assert_context_is_current();
          setup_projection (axis, with_projection);
          draw_plane_primitive (axis, shader_program, with_projection);
          draw_crosshairs (with_projection);
          draw_orientation_labels (with_projection);
          GL::assert_context_is_current();
        }






      }
    }
  }
}


