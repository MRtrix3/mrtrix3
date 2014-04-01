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

#include "gui/mrview/tool/fixel/fixel_image.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        FixelImage::FixelImage (const std::string& filename, Fixel& fixel_tool) :
          Displayable (filename),
          filename (filename),
          fixel_tool (fixel_tool),
          header (filename),
          fixel_data (header),
          fixel_vox (fixel_data),
          header_transform (fixel_vox),
          colourbar_position_index (4),
          slice_fixel_indices (3),
          slice_fixel_sizes (3),
          slice_fixel_counts (3),
          line_length_multiplier (1.0),
          line_length_by_value (false),
          color_type (Value),
          show_colour_bar (true)
          {
            set_allowed_features (true, true, false);
            colourmap = 1;
            alpha = 1.0f;
            set_use_transparency (true);
            colour[0] = colour[1] = colour[2] = 1;
            line_length = 0.45 * static_cast<float>(fixel_vox.vox(0) + fixel_vox.vox(1) + fixel_vox.vox(2)) / 3.0;
            value_min = std::numeric_limits<float>::infinity();
            value_max = -std::numeric_limits<float>::infinity();
            load_image();
          }


        FixelImage::~FixelImage()
        {
          if (vertex_buffer)
            gl::DeleteBuffers (1, &vertex_buffer);
          if (vertex_array_object)
            gl::DeleteVertexArrays (1, &vertex_array_object);
          if (value_buffer)
            gl::DeleteBuffers (1, &value_buffer);
        }


        std::string FixelImage::Shader::vertex_shader_source (const Displayable& fixel)
        {
           std::string source =
               "layout (location = 0) in vec3 vertexposition_modelspace;\n"
               "layout (location = 1) in vec3 previousVertex;\n"
               "layout (location = 2) in vec3 nextVertex;\n"
               "layout (location = 3) in float value;\n"
               "uniform mat4 MVP;\n"
               "uniform float line_length;\n"
               "uniform float max_value;\n"
               "uniform bool line_length_by_value;\n"
               "flat out float value_out;\n"
               "out vec3 fragmentColour;\n";

           switch (color_type) {
             case Direction: break;
             case Colour:
               source += "uniform vec3 const_colour;\n";
               break;
             case Value:
               source += "uniform float offset, scale;\n";
               break;
           }

           source +=
               "void main() {\n"
               "  vec3 dir;\n"
               "  vec3 vertexposition_length_adjusted;\n"
               "  if (isnan (previousVertex.x))\n"
               "    dir = nextVertex - vertexposition_modelspace;\n"
               "  else if (isnan (nextVertex.x))\n"
               "    dir = previousVertex - vertexposition_modelspace;\n"
               "  float length_adjustment;\n"
               "  if (line_length_by_value)\n"
               "    length_adjustment = 0.5 * (1.0 - line_length * (value / max_value));\n"
               "  else\n"
               "    length_adjustment = 0.5 * (1.0 - line_length);\n"
               "  vec3 length_adjustment_vector = length_adjustment * dir;\n"
               "  vertexposition_length_adjusted = vertexposition_modelspace + length_adjustment_vector;\n"
               "  gl_Position =  MVP * vec4 (vertexposition_length_adjusted, 1);\n"
               "  value_out = value;\n";

           switch (color_type) {
             case Colour:
               source +=
                   "  fragmentColour = const_colour;\n";
               break;
             case Value:
               if (!ColourMap::maps[colourmap].special) {
                 source += "  float amplitude = clamp (";
                 if (fixel.scale_inverted())
                   source += "1.0 -";
                 source += " scale * (value - offset), 0.0, 1.0);\n  ";
               }
               source +=
                 std::string ("  vec3 color;\n") +
                 ColourMap::maps[colourmap].mapping +
                 "  fragmentColour = color;\n";
               break;
             case Direction:
               source +=
                 "  fragmentColour = normalize (abs (dir));\n";
               break;
             default:
               break;
           }
           source += "}\n";
           return source;
        }


        std::string FixelImage::Shader::fragment_shader_source (const Displayable& fixel)
        {
          std::string source =
              "in float include; \n"
              "out vec3 color;\n"
              "flat in float value_out;\n"
              "in vec3 fragmentColour;\n";

          if (fixel.use_discard_lower())
            source += "uniform float lower;\n";
          if (fixel.use_discard_upper())
            source += "uniform float upper;\n";

          source +=
              "void main(){\n";

          if (fixel.use_discard_lower())
            source += "  if (value_out < lower) discard;\n";
          if (fixel.use_discard_upper())
            source += "  if (value_out > upper) discard;\n";

          source +=
            std::string("  color = fragmentColour;\n");

          source += "}\n";
          return source;
        }


        bool FixelImage::Shader::need_update (const Displayable& object) const
        {
          const FixelImage& fixel (dynamic_cast<const FixelImage&> (object));
          if (color_type != fixel.color_type)
            return true;
          return Displayable::Shader::need_update (object);
        }


        void FixelImage::Shader::update (const Displayable& object)
        {
          const FixelImage& fixel (dynamic_cast<const FixelImage&> (object));
          do_crop_to_slice = fixel.fixel_tool.do_crop_to_slice;
          color_type = fixel.color_type;
          Displayable::Shader::update (object);
        }


        void FixelImage::render (const Projection& projection, int axis, int slice)
        {
          start (fixel_shader);
          projection.set (fixel_shader);

          gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "line_length"), line_length * line_length_multiplier);

          gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "max_value"), value_max);
          gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "line_length_by_value"), line_length_by_value);

          if (use_discard_lower())
            gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "lower"), lessthan);
          if (use_discard_upper())
            gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "upper"), greaterthan);

          if (color_type == Colour)
            gl::Uniform3fv (gl::GetUniformLocation (fixel_shader, "const_colour"), 1, colour);

          if (fixel_tool.line_opacity < 1.0) {
            gl::Enable (gl::BLEND);
            gl::Disable (gl::DEPTH_TEST);
            gl::DepthMask (gl::FALSE_);
            gl::BlendEquation (gl::FUNC_ADD);
            gl::BlendFunc (gl::CONSTANT_ALPHA, gl::ONE);
            gl::BlendColor (1.0, 1.0, 1.0, fixel_tool.line_opacity);
          } else {
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
          }

          gl::LineWidth (fixel_tool.line_thickness);

          gl::BindVertexArray (vertex_array_object);

          if (!fixel_tool.do_crop_to_slice) {
            for (size_t x = 0; x < slice_fixel_indices[0].size(); ++x)
              gl::MultiDrawArrays (gl::LINE_STRIP, &slice_fixel_indices[0][x][0], &slice_fixel_sizes[0][x][0], slice_fixel_counts[0][x]);
          } else {
            if (slice >= 0 && slice < fixel_vox.dim(axis))
              gl::MultiDrawArrays (gl::LINE_STRIP, &slice_fixel_indices[axis][slice][0], &slice_fixel_sizes[axis][slice][0], slice_fixel_counts[axis][slice]);
          }

          if (fixel_tool.line_opacity < 1.0) {
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
          }

          stop (fixel_shader);
        }


        void FixelImage::load_image ()
        {
          for (size_t dim = 0; dim < 3; ++dim) {
            slice_fixel_indices[dim].resize (fixel_vox.dim(dim));
            slice_fixel_sizes[dim].resize (fixel_vox.dim(dim));
            slice_fixel_counts[dim].resize (fixel_vox.dim(dim));
          }

          MR::Image::LoopInOrder loop (fixel_vox);
          std::vector<Point<float> > buffer;
          std::vector<float> values;
          std::vector<GLint> starts;
          std::vector<GLint> sizes;
          for (loop.start (fixel_vox); loop.ok(); loop.next (fixel_vox)) {
            for (size_t f = 0; f != fixel_vox.value().size(); ++f) {
              if (fixel_vox.value()[f].value > value_max)
                value_max = fixel_vox.value()[f].value;
              if (fixel_vox.value()[f].value < value_min)
                value_min = fixel_vox.value()[f].value;
              header_transform.voxel2scanner (fixel_vox, voxel_pos);
              for (size_t dim = 0; dim < 3; ++dim) {
                slice_fixel_indices[dim][fixel_vox[dim]].push_back (buffer.size());
                slice_fixel_sizes[dim][fixel_vox[dim]].push_back(2);
                slice_fixel_counts[dim][fixel_vox[dim]]++;
              }
              values.push_back (NAN);
              values.push_back (fixel_vox.value()[f].value);
              values.push_back (fixel_vox.value()[f].value);
              buffer.push_back (Point<float>());
              buffer.push_back (voxel_pos + (fixel_vox.value()[f].dir));
              buffer.push_back (voxel_pos + (fixel_vox.value()[f].dir * -1.0));
            }
          }

          this->set_windowing (value_min, value_max);
          greaterthan = value_max;
          lessthan = value_min;
          buffer.push_back (Point<float> ());
          values.push_back (NAN);

          gl::GenBuffers (1, &vertex_buffer);
          gl::BindBuffer (gl::ARRAY_BUFFER, vertex_buffer);
          gl::BufferData (gl::ARRAY_BUFFER, buffer.size() * sizeof(Point<float>), &buffer[0][0], gl::STATIC_DRAW);

          gl::GenVertexArrays (1, &vertex_array_object);
          gl::BindVertexArray (vertex_array_object);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(3*sizeof(float)));
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);
          gl::EnableVertexAttribArray (2);
          gl::VertexAttribPointer (2, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(6*sizeof(float)));

          gl::GenBuffers (1, &value_buffer);
          gl::BindBuffer (gl::ARRAY_BUFFER, value_buffer);
          gl::BufferData (gl::ARRAY_BUFFER, values.size() * sizeof(float), &values[0], gl::STATIC_DRAW);

          gl::EnableVertexAttribArray (3);
          gl::VertexAttribPointer (3, 1, gl::FLOAT, gl::FALSE_, 0, (void*)(sizeof(float)));
        }
      }
    }
  }
}
