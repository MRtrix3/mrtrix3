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


        AbstractFixel::AbstractFixel (const std::string& filename, Vector& fixel_tool) :
          Displayable (filename),
          filename (filename),
          header (filename),
          slice_fixel_indices (3),
          slice_fixel_sizes (3),
          slice_fixel_counts (3),
          fixel_tool (fixel_tool),
          colourbar_position_index (4),
          voxel_size_length_multipler (1.f),
          user_line_length_multiplier (1.f),
          line_thickness (0.005f),
          length_type (Unity),
          colour_type (CValue)
        {
          set_allowed_features (true, true, false);
          colourmap = 1;
          alpha = 1.0f;
          set_use_transparency (true);
          colour[0] = colour[1] = colour[2] = 1;
          value_min = std::numeric_limits<float>::infinity();
          value_max = -std::numeric_limits<float>::infinity();
          voxel_size_length_multipler = 0.45 * (header.vox(0) + header.vox(1) + header.vox(2)) / 3;
        }


        Fixel::Fixel (const std::string& filename, Vector& fixel_tool) :
          AbstractFixel(filename, fixel_tool),
          fixel_data (header),
          fixel_vox (fixel_data),
          header_transform (fixel_vox)
        {
          load_image();
        }


        PackedFixel::PackedFixel (const std::string& filename, Vector& fixel_tool) :
          AbstractFixel (filename, fixel_tool),
          packed_fixel_data (header),
          packed_fixel_vox (packed_fixel_data),
          header_transform (packed_fixel_vox)
        {
          load_image();
        }


        std::string AbstractFixel::Shader::vertex_shader_source (const Displayable&)
        {
          std::string source =
               "layout (location = 0) in vec3 centre;\n"
               "layout (location = 1) in vec3 direction;\n"
               "layout (location = 2) in vec2 fixel_metrics;\n"
               "out vec3 v_dir;"
               "out vec2 v_fixel_metrics;"
               "void main() { "
               "    gl_Position = vec4(centre, 1);\n"
               "    v_dir = direction;"
               "    v_fixel_metrics = fixel_metrics;"
               "}\n";

          return source;
        }


        std::string AbstractFixel::Shader::geometry_shader_source (const Displayable& fixel)
        {
          std::string source =
              "layout(points) in;\n"
              "layout(triangle_strip, max_vertices = 4) out;\n"
              "in vec3 v_dir[];\n"
              "in vec2 v_fixel_metrics[];\n"
              "uniform mat4 MVP;\n"
              "uniform float length_mult;\n"
              "uniform vec3 colourmap_colour;\n"
              "uniform float line_thickness;\n";

          switch (color_type) {
            case Direction: break;
            case CValue:
              source += "uniform float offset, scale;\n";
              break;
          }

          source +=
               "out vec3 fColour;\n"
               "flat out float value_out;\n"
               "void main() {\n";

          // Make sure we pass our output parameters before ending the primitive!
          switch (length_type) {
            case Unity:
              source += "   value_out = v_fixel_metrics[0].y;\n"
                        "   vec4 line_offset = length_mult * vec4 (v_dir[0], 0);\n";
              break;
            case Amplitude:
              source += "   value_out = v_fixel_metrics[0].x;\n"
                        "   vec4 line_offset = length_mult * value_out * vec4 (v_dir[0], 0);\n";
              break;
            case LValue:
              source += "   value_out = v_fixel_metrics[0].y;\n"
                        "   vec4 line_offset = length_mult * value_out * vec4 (v_dir[0], 0);\n";
              break;
          }

          switch (color_type) {
            case CValue:
              if (!ColourMap::maps[colourmap].special) {
                source += "    float amplitude = clamp (";
                if (fixel.scale_inverted())
                  source += "1.0 -";
                source += " scale * (v_fixel_metrics[0].y - offset), 0.0, 1.0);\n";
              }
              source +=
                std::string ("    vec3 color;\n") +
                ColourMap::maps[colourmap].glsl_mapping +
                "   fColour = color;\n";
              break;
            case Direction:
              source +=
                "   fColour = normalize (abs (v_dir[0]));\n";
              break;
            default:
              break;
          }

          source +=
               "    vec4 start = MVP * (gl_in[0].gl_Position - line_offset);\n"
               "    vec4 end = MVP * (gl_in[0].gl_Position + line_offset);\n"
               "    vec4 line = end - start;\n"
               "    vec4 normal =  normalize(vec4(-line.y, line.x, 0.0, 0.0));\n"
               "    vec4 thick_vec =  line_thickness * normal;\n"
               "    gl_Position = start - thick_vec;\n"
               "    EmitVertex();\n"
               "    gl_Position = start + thick_vec;\n"
               "    EmitVertex();\n"
               "    gl_Position = end - thick_vec;\n"
               "    EmitVertex();\n"
               "    gl_Position = end + thick_vec;\n"
               "    EmitVertex();\n"
               "    EndPrimitive();\n"
               "}\n";

          return source;
        }


        std::string AbstractFixel::Shader::fragment_shader_source (const Displayable& fixel)
        {
          std::string source =
              "out vec3 outColour;\n"
              "in vec3 fColour;\n"
              "flat in float value_out;\n";

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
            std::string("  outColour = fColour;\n");

          source += "}\n";

          return source;
        }


        bool AbstractFixel::Shader::need_update (const Displayable& object) const
        {
          const AbstractFixel& fixel (dynamic_cast<const AbstractFixel&> (object));
          if (color_type != fixel.colour_type)
            return true;
          if (length_type != fixel.length_type)
            return true;
          return Displayable::Shader::need_update (object);
        }


        void AbstractFixel::Shader::update (const Displayable& object)
        {
          const AbstractFixel& fixel (dynamic_cast<const AbstractFixel&> (object));
          do_crop_to_slice = fixel.fixel_tool.do_crop_to_slice;
          color_type = fixel.colour_type;
          length_type = fixel.length_type;
          Displayable::Shader::update (object);
        }


        void AbstractFixel::render (const Projection& projection, int axis, int slice)
        {

          if (fixel_tool.do_crop_to_slice && (slice < 0 || slice >= header.dim(axis)))
            return;

          start (fixel_shader);
          projection.set (fixel_shader);

          gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "length_mult"), voxel_size_length_multipler * user_line_length_multiplier);
          gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "line_thickness"), line_thickness);

          if (use_discard_lower())
            gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "lower"), lessthan);
          if (use_discard_upper())
            gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "upper"), greaterthan);

          if (ColourMap::maps[colourmap].is_colour)
            gl::Uniform3f (gl::GetUniformLocation (fixel_shader, "colourmap_colour"),
                colour[0]/255.0f, colour[1]/255.0f, colour[2]/255.0f);

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

          vertex_array_object.bind();

          if (!fixel_tool.do_crop_to_slice) {
            for (size_t x = 0; x < slice_fixel_indices[0].size(); ++x)
              gl::MultiDrawArrays (gl::POINTS, &slice_fixel_indices[0][x][0], &slice_fixel_sizes[0][x][0], slice_fixel_counts[0][x]);
          } else {
            gl::MultiDrawArrays (gl::POINTS, &slice_fixel_indices[axis][slice][0], &slice_fixel_sizes[axis][slice][0], slice_fixel_counts[axis][slice]);
          }

          if (fixel_tool.line_opacity < 1.0) {
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
          }

          stop (fixel_shader);
        }


        void AbstractFixel::load_image ()
        {
          load_image_buffer ();

          // voxel centres
          vertex_buffer.gen();
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, buffer_pos.size() * sizeof(Point<float>), &buffer_pos[0][0], gl::STATIC_DRAW);
          vertex_array_object.gen();
          vertex_array_object.bind();
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          // fixel directions
          direction_buffer.gen();
          direction_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, buffer_dir.size() * sizeof(Point<float>), &buffer_dir[0][0], gl::STATIC_DRAW);
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          // fixel sizes and values
          value_buffer.gen();
          value_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, buffer_val.size() * sizeof(float), &buffer_val[0], gl::STATIC_DRAW);
          gl::EnableVertexAttribArray (2);
          gl::VertexAttribPointer (2, 2, gl::FLOAT, gl::FALSE_, 0, (void*)0);
        }


        void Fixel::load_image_buffer()
        {
          for (size_t dim = 0; dim < 3; ++dim) {
            ssize_t dim_size = fixel_vox.dim(dim);
            slice_fixel_indices[dim].resize (dim_size);
            slice_fixel_sizes[dim].resize (dim_size);
            slice_fixel_counts[dim].resize (dim_size, 0);
          }

          MR::Image::LoopInOrder loop (fixel_vox);

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
              header_transform.voxel2scanner (fixel_vox, voxel_pos);
              buffer_pos.push_back (voxel_pos);
              buffer_dir.push_back (fixel_vox.value()[f].dir);
              buffer_val.push_back (fixel_vox.value()[f].size);
              buffer_val.push_back (fixel_vox.value()[f].value);

              for (size_t dim = 0; dim < 3; ++dim) {
                slice_fixel_indices[dim][fixel_vox[dim]].push_back (buffer_pos.size() - 1);
                slice_fixel_sizes[dim][fixel_vox[dim]].push_back(1);
                slice_fixel_counts[dim][fixel_vox[dim]]++;
              }
            }
          }

          this->set_windowing (value_min, value_max);
          greaterthan = value_max;
          lessthan = value_min;
        }


        void PackedFixel::load_image_buffer()
        {
          size_t ndim = packed_fixel_vox.ndim();

          if (ndim != 4)
            throw InvalidImageException ("Vector image " + filename
                                         + " should contain 4 dimensions. Instead "
                                         + str(ndim) + " found.");

          size_t dim4_len = packed_fixel_vox.dim(3);

          if (dim4_len % 3)
            throw InvalidImageException ("Expecting 4th-dimension size of vector image "
                                         + filename + " to be a multiple of 3. Instead "
                                         + str(dim4_len) + " entries found.");

          for (size_t dim = 0; dim < 3; ++dim) {
            slice_fixel_indices[dim].resize (packed_fixel_vox.dim(dim));
            slice_fixel_sizes[dim].resize (packed_fixel_vox.dim(dim));
            slice_fixel_counts[dim].resize (packed_fixel_vox.dim(dim), 0);
          }

          MR::Image::LoopInOrder loop (packed_fixel_vox, 0, 3);

          Point<float> voxel_pos;
          size_t n_fixels = dim4_len / 3;

          for (auto l = loop (packed_fixel_vox); l; ++l) {
            for (size_t f = 0; f < n_fixels; ++f) {

              // Fetch the vector components
              float x_comp, y_comp, z_comp;
              packed_fixel_vox[3] = 3*f;
              x_comp = packed_fixel_vox.value();
              packed_fixel_vox[3]++;
              y_comp = packed_fixel_vox.value();
              packed_fixel_vox[3]++;
              z_comp = packed_fixel_vox.value();

              Point<> vector(x_comp, y_comp, z_comp);
              float length = vector.norm();
              value_min = std::min(value_min, length);
              value_max = std::max(value_max, length);

              header_transform.voxel2scanner (packed_fixel_vox, voxel_pos);
              buffer_pos.push_back (voxel_pos);
              buffer_dir.push_back (vector.normalise());

              // Use the vector length to represent both fixel amplitude and value
              buffer_val.push_back (length);
              buffer_val.push_back (length);

              for (size_t dim = 0; dim < 3; ++dim) {
                slice_fixel_indices[dim][packed_fixel_vox[dim]].push_back (buffer_pos.size() - 1);
                slice_fixel_sizes[dim][packed_fixel_vox[dim]].push_back(1);
                slice_fixel_counts[dim][packed_fixel_vox[dim]]++;
              }
            }
          }

          this->set_windowing (value_min, value_max);
          greaterthan = value_max;
          lessthan = value_min;
        }

      }
    }
  }
}
