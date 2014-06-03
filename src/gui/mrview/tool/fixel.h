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
#include "gui/mrview/tool/vector.h"
#include "image/loop.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        enum ColourType { Value, Direction, Colour };

        class Fixel : public Displayable {
          public:
            Fixel (const std::string& filename, Vector& fixel_tool);

            ~Fixel();

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


              void render (const Projection& projection, int axis, int slice);

              void renderColourBar (const Projection& transform) {
                if (color_type == Value && show_colour_bar)
                  colourbar_renderer.render (transform, *this, colourbar_position_index, this->scale_inverted());
              }

              void load_image ();

              void set_colour (float c[3]) {
                colour[0] = c[0];
                colour[1] = c[1];
                colour[2] = c[2];
              }

              void set_line_length_multiplier (float value) {
                line_length_multiplier = value;
              }

              float get_line_length_multiplier () const {
                return line_length_multiplier;
              }

              void set_line_length_by_value (bool value) {
                scale_line_length_by_value = value;
              }

              bool get_line_length_by_value () const {
                return scale_line_length_by_value;
              }

              void set_colour_type (ColourType value) {
                color_type = value;
              }

              void set_show_colour_bar (bool value) {
                show_colour_bar = value;
              }


            private:
              std::string filename;
              Vector& fixel_tool;
              MR::Image::Header header;
              MR::Image::BufferSparse<MR::Image::Sparse::FixelMetric> fixel_data;
              MR::Image::BufferSparse<MR::Image::Sparse::FixelMetric>::voxel_type fixel_vox;
              MR::Image::Transform header_transform;
              ColourMap::Renderer colourbar_renderer;
              int colourbar_position_index;
              GLuint vertex_buffer;
              GLuint vertex_array_object;
              GLuint value_buffer;
              GLuint value_array_object;
              std::vector<std::vector<std::vector<GLint> > > slice_fixel_indices;
              std::vector<std::vector<std::vector<GLsizei> > > slice_fixel_sizes;
              std::vector<std::vector<GLsizei> > slice_fixel_counts;
              float colour[3];
              float line_length;
              float line_length_multiplier;
              bool  scale_line_length_by_value;
              ColourType color_type;
              bool show_colour_bar;
        };

      }
    }
  }
}
#endif
