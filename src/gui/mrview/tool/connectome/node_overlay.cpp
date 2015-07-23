/*
   Copyright 2014 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2015.

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


        NodeOverlay::NodeOverlay (const MR::Image::Info& info) :
            MR::GUI::MRView::ImageBase (info),
            data (info),
            need_update (true)
        {
          position.assign       (3, -1);
          set_interpolate       (false);
          set_colourmap         (5);
          set_min_max           (std::numeric_limits<float>::min(), 1.0f);
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

          if (position[plane] == slice && !need_update)
            return;
          position[plane] = slice;

          int x, y;
          get_axes (plane, x, y);
          const ssize_t xdim = _info.dim (x), ydim = _info.dim (y);

          std::vector<float> texture_data;
          auto vox = data.voxel();
          texture_data.resize (4*xdim*ydim, 0.0f);
          if (position[plane] >= 0 && position[plane] < _info.dim (plane)) {
            vox[plane] = slice;
            for (vox[y] = 0; vox[y] < ydim; ++vox[y]) {
              for (vox[x] = 0; vox[x] < xdim; ++vox[x]) {
                for (vox[3] = 0; vox[3] != 4; ++vox[3]) {
                  texture_data[4*(vox[x]+vox[y]*xdim) + vox[3]] = vox.value();
            } } }
          }

          gl::TexImage3D (gl::TEXTURE_3D, 0, internal_format, xdim, ydim, 1, 0, format, type, reinterpret_cast<void*> (&texture_data[0]));
          need_update = false;
        }

        void NodeOverlay::update_texture3D()
        {
          bind();
          allocate();
          if (!need_update) return;
          upload_data ({ { 0, 0, 0 } }, { { data.dim(0), data.dim(1), data.dim(2) } }, reinterpret_cast<void*> (data.address(0)));
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





