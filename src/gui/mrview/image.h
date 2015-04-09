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

#ifndef __gui_mrview_image_h__
#define __gui_mrview_image_h__

#include "gui/opengl/gl.h"
#include "gui/mrview/volume.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "math/versor.h"
#include "image/interp/linear.h"
#include "image/interp/nearest.h"


namespace MR
{
  class ProgressBar;

  namespace GUI
  {
    class Projection;

    namespace MRView
    {

      class Window;

      class Image : public Volume
      {
        public:
          Image (const MR::Image::Header& image_header);
          Image (Window& parent, const MR::Image::Header& image_header);

          MR::Image::Header& header () { return buffer; }
          const MR::Image::Header& header () const { return buffer; }

          void update_texture2D (int plane, int slice);
          void update_texture3D ();

          void render2D (Displayable::Shader& shader_program, const Projection& projection, int plane, int slice);
          void render3D (Displayable::Shader& shader_program, const Projection& projection, float depth);

          void request_render_colourbar(DisplayableVisitor& visitor, const Projection& projection) override
          { if(show_colour_bar) visitor.render_image_colourbar(*this, projection); }

          void get_axes (int plane, int& x, int& y) {
            if (plane) {
              if (plane == 1) {
                x = 0;
                y = 2;
              }
              else {
                x = 0;
                y = 1;
              }
            }
            else {
              x = 1;
              y = 2;
            }
          }

          typedef MR::Image::Buffer<cfloat> BufferType;
          typedef BufferType::voxel_type VoxelType;
          typedef MR::Image::Interp::Linear<VoxelType> InterpVoxelType;

        private:
          BufferType buffer;
          MR::Image::Interp::Nearest<VoxelType> nearest_interp;

        public:
          InterpVoxelType interp;
          VoxelType& voxel () { return interp; }
          cfloat trilinear_value(const Point<float> &scanner_point) {
            if(interp.scanner(scanner_point)) { return cfloat(NAN, NAN); }
            return interp.value();
          }
          cfloat nearest_neighbour_value(const Point<float> &scanner_point) {
            if(nearest_interp.scanner(scanner_point)) { return cfloat(NAN, NAN); }
            return nearest_interp.value();
          }

        private:
          GL::Texture texture2D[3];
          std::vector<ssize_t> position;

          bool volume_unchanged ();
          size_t guess_colourmap () const;

          template <typename T> void copy_texture_3D ();
          void copy_texture_3D_complex ();

      };


    }
  }
}

#endif

