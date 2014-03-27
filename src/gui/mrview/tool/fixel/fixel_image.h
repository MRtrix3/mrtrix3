/*
   Copyright 2014 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt 2014.

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
#ifndef __gui_mrview_tool_fixel_fixelimage_h__
#define __gui_mrview_tool_fixel_fixelimage_h__

#include "gui/mrview/displayable.h"
#include "image/header.h"
#include "image/buffer_sparse.h"
#include "image/sparse/fixel_metric.h"
#include "image/sparse/voxel.h"
#include "image/transform.h"
#include "gui/mrview/tool/fixel/fixel.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        enum ColourType { Value, Direction, Colour };

        class FixelImage : public Displayable {
          public:
            FixelImage (const std::string& filename, Fixel& fixel_tool) :
              Displayable (filename),
              show_colour_bar (true),
              color_type (Colour),
              filename (filename),
              fixel_tool (fixel_tool),
              header (filename),
              fixel_data (header),
              fixel_vox (fixel_data),
              transform (fixel_vox),
              colourbar_position_index (4)
              {
                set_allowed_features (true, true, false);
                colourmap = 1;
                alpha = 1.0f;
                set_use_transparency (true);
                colour[0] = colour[1] = colour[2] = 1;
                line_length = 0.5 * static_cast<float>(fixel_vox.vox(0) + fixel_vox.vox(1) + fixel_vox.vox(2)) / 3.0;
              }


              class Shader : public Displayable::Shader {
                  public:
                    Shader () : do_crop_to_slice (false), color_type (Direction) { }
                    virtual std::string vertex_shader_source (const Displayable& fixel_image);
                  virtual std::string fragment_shader_source (const Displayable& fixel_image);
                  virtual bool need_update (const Displayable& object) const;
                  virtual void update (const Displayable& object);
                protected:
                  bool do_crop_to_slice;
                  ColourType color_type;
              } fixel_shader;


              void render (const Projection& transform, bool is_3D, int plane, int slice);

              void renderColourBar (const Projection& transform) {
                if (color_type == Value && show_colour_bar)
                  colourbar_renderer.render (transform, *this, colourbar_position_index, this->scale_inverted());
              }

              void set_colour (float c[3])
              {
                colour[0] = c[0];
                colour[1] = c[1];
                colour[2] = c[2];
              }

              bool show_colour_bar;
              ColourType color_type;
              float colour[3];
              float line_length;

            private:
              std::string filename;
              Fixel& fixel_tool;
              MR::Image::Header header;
              MR::Image::BufferSparse<MR::Image::Sparse::FixelMetric> fixel_data;
              MR::Image::BufferSparse<MR::Image::Sparse::FixelMetric>::voxel_type fixel_vox;
              MR::Image::Transform transform;
              Point<float> voxel_pos;
              ColourMap::Renderer colourbar_renderer;
              int colourbar_position_index;
        };

      }
    }
  }
}
#endif
