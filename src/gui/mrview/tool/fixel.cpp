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

#include "gui/mrview/tool/fixel.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        Fixel::Fixel (const std::string& filename, Vector& fixel_tool) :
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
          voxel_size_length_multipler (1.0),
          user_line_length_multiplier (1.0),
          length_type (Unity),
          colour_type (CValue),
          show_colour_bar (true)
          {
            set_allowed_features (true, true, false);
            colourmap = 1;
            alpha = 1.0f;
            set_use_transparency (true);
            colour[0] = colour[1] = colour[2] = 1;
            value_min = std::numeric_limits<float>::infinity();
            value_max = -std::numeric_limits<float>::infinity();
            voxel_size_length_multipler = 0.45 * (header.vox(0) + header.vox(1) + header.vox(2)) / 3;
            load_image();
          }


        std::string Fixel::Shader::vertex_shader_source (const Displayable& fixel)
        {
           std::string source =
               "layout (location = 0) in vec3 this_vertex;\n"
               "layout (location = 1) in vec3 prev_vertex;\n"
               "layout (location = 2) in vec3 next_vertex;\n"
               "layout (location = 3) in float this_value;\n"
               "layout (location = 4) in float prev_value;\n"
               "layout (location = 5) in float next_value;\n"
               "uniform mat4 MVP;\n"
               "uniform float length_mult;\n"
               "flat out float value_out;\n"
               "out vec3 fragmentColour;\n";

           switch (color_type) {
             case Direction: break;
             case Manual:
               source += "uniform vec3 const_colour;\n";
               break;
             case CValue:
               source += "uniform float offset, scale;\n";
               break;
           }

           source +=
               "void main() {\n"
               "  vec3 centre = this_vertex;\n"
               "  vec3 dir = next_vertex;\n"
               "  float amp = this_value;\n"
               "  float value = next_value;\n"
               "  if ((gl_VertexID % 2) > 0) {\n"
               "    centre = prev_vertex;\n"
               "    dir = -this_vertex;\n"
               "    amp = prev_value;\n"
               "    value = this_value;\n"
               "  }\n"
               "  value_out = value;\n";

           switch (length_type) {
             case Unity:      source += "  gl_Position = MVP * vec4 (centre + length_mult * dir, 1);\n";         break;
             case Amplitude:  source += "  gl_Position = MVP * vec4 (centre + length_mult * amp * dir, 1);\n";   break;
             case LValue:     source += "  gl_Position = MVP * vec4 (centre + length_mult * value * dir, 1);\n"; break;
           }

           switch (color_type) {
             case Manual:
               source +=
                   "  fragmentColour = const_colour;\n";
               break;
             case CValue:
               if (!ColourMap::maps[colourmap].special) {
                 source += "  float amplitude = clamp (";
                 if (fixel.scale_inverted())
                   source += "1.0 -";
                 source += " scale * (value_out - offset), 0.0, 1.0);\n";
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


        std::string Fixel::Shader::fragment_shader_source (const Displayable& fixel)
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


        bool Fixel::Shader::need_update (const Displayable& object) const
        {
          const Fixel& fixel (dynamic_cast<const Fixel&> (object));
          if (color_type != fixel.colour_type)
            return true;
          if (length_type != fixel.length_type)
            return true;
          return Displayable::Shader::need_update (object);
        }


        void Fixel::Shader::update (const Displayable& object)
        {
          const Fixel& fixel (dynamic_cast<const Fixel&> (object));
          do_crop_to_slice = fixel.fixel_tool.do_crop_to_slice;
          color_type = fixel.colour_type;
          length_type = fixel.length_type;
          Displayable::Shader::update (object);
        }


        void Fixel::render (const Projection& projection, int axis, int slice)
        {

          if (fixel_tool.do_crop_to_slice && (slice < 0 || slice >= header.dim(axis)))
            return;

          start (fixel_shader);
          projection.set (fixel_shader);

          gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "length_mult"), voxel_size_length_multipler * user_line_length_multiplier);

          if (use_discard_lower())
            gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "lower"), lessthan);
          if (use_discard_upper())
            gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "upper"), greaterthan);

          if (colour_type == Manual)
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

          vertex_array_object.bind();

          if (!fixel_tool.do_crop_to_slice) {
            for (size_t x = 0; x < slice_fixel_indices[0].size(); ++x)
              gl::MultiDrawArrays (gl::LINES, &slice_fixel_indices[0][x][0], &slice_fixel_sizes[0][x][0], slice_fixel_counts[0][x]);
          } else {
            gl::MultiDrawArrays (gl::LINES, &slice_fixel_indices[axis][slice][0], &slice_fixel_sizes[axis][slice][0], slice_fixel_counts[axis][slice]);
          }

          if (fixel_tool.line_opacity < 1.0) {
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
          }

          stop (fixel_shader);
        }


        void Fixel::load_image ()
        {
          for (size_t dim = 0; dim < 3; ++dim) {
            slice_fixel_indices[dim].resize (fixel_vox.dim(dim));
            slice_fixel_sizes[dim].resize (fixel_vox.dim(dim));
            slice_fixel_counts[dim].resize (fixel_vox.dim(dim), 0);
          }

          MR::Image::LoopInOrder loop (fixel_vox);
          std::vector<Point<float> > buffer_dir;
          std::vector<float> buffer_val;
          buffer_dir.push_back(Point<float>());
          buffer_val.push_back(NAN);
          Point<float> voxel_pos;
          for (auto l = loop (fixel_vox); l; ++l) {
            for (size_t f = 0; f != fixel_vox.value().size(); ++f) {
              if (fixel_vox.value()[f].value > value_max)
                value_max = fixel_vox.value()[f].value;
              if (fixel_vox.value()[f].value < value_min)
                value_min = fixel_vox.value()[f].value;
            }
          }
          for (loop.start (fixel_vox); loop.ok(); loop.next (fixel_vox)) {
            for (size_t f = 0; f != fixel_vox.value().size(); ++f) {
              for (size_t dim = 0; dim < 3; ++dim) {
                slice_fixel_indices[dim][fixel_vox[dim]].push_back (buffer_dir.size() - 1);
                slice_fixel_sizes[dim][fixel_vox[dim]].push_back(2);
                slice_fixel_counts[dim][fixel_vox[dim]]++;
              }
              header_transform.voxel2scanner (fixel_vox, voxel_pos);
              buffer_dir.push_back (voxel_pos);
              buffer_dir.push_back (fixel_vox.value()[f].dir);
              buffer_val.push_back (fixel_vox.value()[f].size);
              buffer_val.push_back (fixel_vox.value()[f].value / value_max);
            }
          }
          buffer_dir.push_back (Point<float>());
          buffer_val.push_back (NAN);
          this->set_windowing (value_min, value_max);
          greaterthan = value_max;
          lessthan = value_min;

          // voxel centres and fixel directions
          vertex_buffer.gen();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, buffer_dir.size() * sizeof(Point<float>), &buffer_dir[0][0], gl::STATIC_DRAW);
          vertex_array_object.gen();
          vertex_array_object.bind();
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(3*sizeof(float)));
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);
          gl::EnableVertexAttribArray (2);
          gl::VertexAttribPointer (2, 3, gl::FLOAT, gl::FALSE_, 0, (void*)(6*sizeof(float)));

          // fixel sizes and values
          value_buffer.gen();
          value_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, buffer_val.size() * sizeof(float), &buffer_val[0], gl::STATIC_DRAW);
          gl::EnableVertexAttribArray (3);
          gl::VertexAttribPointer (3, 1, gl::FLOAT, gl::FALSE_, 0, (void*)(sizeof(float)));
          gl::EnableVertexAttribArray (4);
          gl::VertexAttribPointer (4, 1, gl::FLOAT, gl::FALSE_, 0, (void*)0);
          gl::EnableVertexAttribArray (5);
          gl::VertexAttribPointer (5, 1, gl::FLOAT, gl::FALSE_, 0, (void*)(2*sizeof(float)));
        }
      }
    }
  }
}
