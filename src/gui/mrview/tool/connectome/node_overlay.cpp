/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "gui/mrview/tool/connectome/node_overlay.h"

#include <limits>
#include <vector>

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        NodeOverlay::NodeOverlay (MR::Header&& H) :
            MR::GUI::MRView::ImageBase (std::move (H)),
            data (MR::Image<float>::scratch (header(), "node overlay scratch image")),
            need_update (true)
        {
          tex_positions.assign  (3, -1);
          set_interpolate       (false);
          set_colourmap         (5);
          value_min = 0.0f; value_max = 1.0f;
          min_max_set           ();
          set_allowed_features  (true, true, true);
          set_use_discard_lower (true);
          set_use_discard_upper (false);
          set_use_transparency  (true);
          set_invert_scale      (false);
          set_interpolate       (false);
          transparent_intensity = opaque_intensity = lessthan = std::numeric_limits<float>::min();
          alpha = 1.0f;
          type = gl::FLOAT;
          format = gl::RGBA;
          internal_format = gl::RGBA32F;
        }




        void NodeOverlay::update_texture2D (const int plane, const int slice)
        {
          // Suspect this should never be run...
          // Only time this could conceptually be used is if activation of the connectome tool
          // 'overhauled' the main image, and the camera & focus plane were set based on the
          // parcellation image
          assert (0);
          if (!texture2D[plane])
            texture2D[plane].gen (gl::TEXTURE_3D);
          texture2D[plane].bind();
          gl::PixelStorei (gl::UNPACK_ALIGNMENT, 1);
          texture2D[plane].set_interp (interpolation);

          if (tex_positions[plane] == slice && !need_update)
            return;
          tex_positions[plane] = slice;

          int x, y;
          get_axes (plane, x, y);
          const ssize_t xsize = data.size (x), ysize = data.size (y);

          std::vector<float> texture_data;
          texture_data.resize (4*xsize*ysize, 0.0f);
          if (tex_positions[plane] >= 0 && tex_positions[plane] < data.size (plane)) {
            data.index(plane) = slice;
            for (data.index(y) = 0; data.index(y) < ysize; ++data.index(y)) {
              for (data.index(x) = 0; data.index(x) < xsize; ++data.index(x)) {
                for (data.index(3) = 0; data.index(3) != 4; ++data.index(3)) {
                  texture_data[4*(data.index(x)+data.index(y)*xsize) + data.index(3)] = data.value();
            } } }
          }

          gl::TexImage3D (gl::TEXTURE_3D, 0, internal_format, xsize, ysize, 1, 0, format, type, reinterpret_cast<void*> (&texture_data[0]));
          need_update = false;
        }

        void NodeOverlay::update_texture3D()
        {
          bind();
          allocate();
          if (!need_update) return;
          // FIXME No longer have access to the raw image data pointer!
          //upload_data ({ { 0, 0, 0 } }, { { data.size(0), data.size(1), data.size(2) } }, reinterpret_cast<void*> (data.address(0)));
          need_update = false;
        }




        std::string NodeOverlay::Shader::vertex_shader_source (const Displayable&)
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


        std::string NodeOverlay::Shader::fragment_shader_source (const Displayable& object)
        {
          assert (object.colourmap == 5);
          std::string source = object.declare_shader_variables () +
            "uniform sampler3D tex;\n"
            "in vec3 texcoord;\n"
            "out vec4 color;\n"
            "void main() {\n"
            "  if (texcoord.s < 0.0 || texcoord.s > 1.0 ||\n"
            "      texcoord.t < 0.0 || texcoord.t > 1.0 ||\n"
            "      texcoord.p < 0.0 || texcoord.p > 1.0) discard;\n"
            "  color = texture (tex, texcoord.stp);\n"
            "  color.a = color.a * alpha;\n";
          source += "  " + std::string (ColourMap::maps[object.colourmap].glsl_mapping);
          source += "}\n";
          return source;
        }


      }
    }
  }
}





