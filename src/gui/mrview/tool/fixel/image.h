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
#ifndef __gui_mrview_tool_fixel_imageh__
#define __gui_mrview_tool_fixel_image_h__

#include "gui/mrview/displayable.h"
#include "image/header.h"
#include "image/buffer_sparse.h"
#include "image/sparse/fixel_metric.h"
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
              color_type (Value),
              filename (filename),
              fixel_tool (fixel_tool),
              header (filename),
              fixel_data (header),
              colourbar_position_index (4)
              {
                set_allowed_features (true, true, false);
                colourmap = 1;
                alpha = 1.0f;
                set_use_transparency (true);
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


              void render (const Projection& transform, bool is_3D, int plane, int slice) {

                //              start (fixel_shader);
                //              transform.set (fixel_shader);
                //
                //              if (color_type == ScalarFile) {
                //                if (use_discard_lower())
                //                  gl::Uniform1f (gl::GetUniformLocation (track_shader, "lower"), lessthan);
                //                if (use_discard_upper())
                //                  gl::Uniform1f (gl::GetUniformLocation (track_shader, "upper"), greaterthan);
                //              }
                //              else if (color_type == Colour)
                //                gl::Uniform3fv (gl::GetUniformLocation (track_shader, "const_colour"), 1, colour);
                //
                //              if (fixel_tool.line_opacity < 1.0) {
                //                gl::Enable (gl::BLEND);
                //                gl::Disable (gl::DEPTH_TEST);
                //                gl::DepthMask (gl::FALSE_);
                //                gl::BlendEquation (gl::FUNC_ADD);
                //                gl::BlendFunc (gl::CONSTANT_ALPHA, gl::ONE);
                //                gl::BlendColor (1.0, 1.0, 1.0, fixel_tool.line_opacity);
                //              } else {
                //                gl::Disable (gl::BLEND);
                //                gl::Enable (gl::DEPTH_TEST);
                //                gl::DepthMask (gl::TRUE_);
                //              }
                //
                //              gl::LineWidth (fixel_tool.line_thickness);
                //
                ////              for (size_t buf = 0; buf < vertex_buffers.size(); ++buf) {
                ////                gl::BindVertexArray (vertex_array_objects[buf]);
                ////                gl::MultiDrawArrays (gl::LINE_STRIP, &track_starts[buf][0], &track_sizes[buf][0], num_tracks_per_buffer[buf]);
                ////              }
                //
                //              if (fixel_tool.line_opacity < 1.0) {
                //                gl::Disable (gl::BLEND);
                //                gl::Enable (gl::DEPTH_TEST);
                //                gl::DepthMask (gl::TRUE_);
                //              }
                //
                //              stop (track_shader);
              }



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

            private:
              std::string filename;
              Fixel& fixel_tool;
              MR::Image::Header header;
              MR::Image::BufferSparse<MR::Image::Sparse::FixelMetric> fixel_data;
              ColourMap::Renderer colourbar_renderer;
              int colourbar_position_index;
        };

      }
    }
  }
}
#endif
