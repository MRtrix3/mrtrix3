/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "gui/mrview/tool/vector/fixel.h"


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
          header (MR::Header::open (filename)),
          slice_fixel_indices (3),
          slice_fixel_sizes (3),
          slice_fixel_counts (3),
          colour_type (Direction),
          scale_type (Unity),
          colour_type_index (0),
          scale_type_index (0),
          threshold_type_index (0),
          fixel_tool (fixel_tool),
          voxel_size_length_multipler (1.f),
          user_line_length_multiplier (1.f),
          line_thickness (0.0015f)
        {
          set_allowed_features (true, true, false);
          colourmap = 1;
          alpha = 1.0f;
          set_use_transparency (true);
          colour[0] = colour[1] = colour[2] = 1;
          value_min = std::numeric_limits<float>::infinity();
          value_max = -std::numeric_limits<float>::infinity();
          voxel_size_length_multipler = 0.45 * (header.spacing(0) + header.spacing(1) + header.spacing(2)) / 3;
        }

        AbstractFixel::~AbstractFixel()
        {
          MRView::GrabContext context;
          vertex_buffer.clear ();
          direction_buffer.clear ();
          vertex_array_object.clear ();
          value_buffer.clear ();
          regular_grid_vao.clear ();
          regular_grid_vertex_buffer.clear ();
          regular_grid_dir_buffer.clear ();
          regular_grid_colour_buffer.clear ();
          regular_grid_val_buffer.clear ();
        }

        std::string AbstractFixel::Shader::vertex_shader_source (const Displayable&)
        {
          std::string source =
               "layout (location = 0) in vec3 centre;\n"
               "layout (location = 1) in vec3 direction;\n"
               "layout (location = 2) in float fixel_scale;\n"
               "layout (location = 3) in float fixel_colour;\n"
               "layout (location = 4) in float fixel_thresh;\n"
               "out vec3 v_dir;"
               "out float v_scale;"
               "out float v_colour;"
               "out float v_threshold;"
               "void main() { "
               "    gl_Position = vec4(centre, 1);\n"
               "    v_dir = direction;\n"
               "    v_scale = fixel_scale;\n"
               "    v_colour = fixel_colour;\n"
               "    v_threshold = fixel_thresh;\n"
               "}\n";

          return source;
        }


        std::string AbstractFixel::Shader::geometry_shader_source (const Displayable& fixel)
        {
          std::string source =
              "layout(points) in;\n"
              "layout(triangle_strip, max_vertices = 4) out;\n"
              "in vec3 v_dir[];\n"
              "in float v_colour[];\n"
              "in float v_scale[];\n"
              "in float v_threshold[];\n"
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

          if (fixel.use_discard_lower())
            source += "uniform float lower;\n";
          if (fixel.use_discard_upper())
            source += "uniform float upper;\n";

          source +=
               "flat out vec3 fColour;\n"
               "void main() {\n";

          if (fixel.use_discard_lower())
            source += "  if (v_threshold[0] < lower) return;\n";
          if (fixel.use_discard_upper())
            source += "  if (v_threshold[0] > upper) return;\n";

          switch (scale_type) {
            case Unity:
              source += "   vec4 line_offset = length_mult * vec4 (v_dir[0], 0);\n";
              break;
            case Value:
              source += "   vec4 line_offset = length_mult * v_scale[0] * vec4 (v_dir[0], 0);\n";
              break;
          }

          switch (color_type) {
            case CValue:
              if (!ColourMap::maps[colourmap].special) {
                source += "    float amplitude = clamp (";
                if (fixel.scale_inverted())
                  source += "1.0 -";
                source += " scale * (v_colour[0] - offset), 0.0, 1.0);\n";
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


        std::string AbstractFixel::Shader::fragment_shader_source (const Displayable&)
        {
          std::string source =
              "out vec3 outColour;\n"
              "flat in vec3 fColour;\n"
              "void main(){\n"
              "  outColour = fColour;\n"
              "}\n";
          return source;
        }


        bool AbstractFixel::Shader::need_update (const Displayable& object) const
        {
          const AbstractFixel& fixel (dynamic_cast<const AbstractFixel&> (object));
          if (color_type != fixel.colour_type)
            return true;
          else if (scale_type != fixel.scale_type)
            return true;
          return Displayable::Shader::need_update (object);
        }


        void AbstractFixel::Shader::update (const Displayable& object)
        {
          const AbstractFixel& fixel (dynamic_cast<const AbstractFixel&> (object));
          do_crop_to_slice = fixel.fixel_tool.do_crop_to_slice;
          color_type = fixel.colour_type;
          scale_type = fixel.scale_type;
          Displayable::Shader::update (object);
        }


        void AbstractFixel::render (const Projection& projection)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          start (fixel_shader);
          projection.set (fixel_shader);

          update_image_buffers ();

          FixelValue& fixel_threshold = current_fixel_threshold_state ();

          gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "length_mult"), voxel_size_length_multipler * user_line_length_multiplier);
          gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "line_thickness"), line_thickness);

          if (use_discard_lower())
            gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "lower"), fixel_threshold.lessthan);
          if (use_discard_upper())
            gl::Uniform1f (gl::GetUniformLocation (fixel_shader, "upper"), fixel_threshold.greaterthan);

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

          if (!fixel_tool.do_crop_to_slice) {
            vertex_array_object.bind();
            for (size_t x = 0, N = slice_fixel_indices[0].size(); x < N; ++x) {
              if (slice_fixel_counts[0][x])
                gl::MultiDrawArrays (gl::POINTS, &slice_fixel_indices[0][x][0], &slice_fixel_sizes[0][x][0], slice_fixel_counts[0][x]);
            }
          } else {
            request_update_interp_image_buffer (projection);

            if (GLsizei points_count = regular_grid_buffer_pos.size())
              gl::DrawArrays(gl::POINTS, 0, points_count);
          }

          if (fixel_tool.line_opacity < 1.0) {
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
          }

          stop (fixel_shader);
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void AbstractFixel::update_image_buffers ()
        {
          if (dir_buffer_dirty)
            reload_directions_buffer ();

          if (value_buffer_dirty)
            reload_values_buffer ();

          if (colour_buffer_dirty)
            reload_colours_buffer ();

          if (threshold_buffer_dirty)
            reload_threshold_buffer ();

          dir_buffer_dirty = false;
          value_buffer_dirty = false;
          colour_buffer_dirty = false;
          threshold_buffer_dirty = false;
        }


        void AbstractFixel::update_interp_image_buffer (const Projection& projection,
                                                        const MR::Header &fixel_header,
                                                        const MR::Transform &transform)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          // Code below "inspired" by ODF::draw
          Eigen::Vector3f p (Window::main->target());
          p += projection.screen_normal() * (projection.screen_normal().dot (Window::main->focus() - p));
          p = transform.scanner2voxel.cast<float>() * p;

          if (fixel_tool.do_lock_to_grid) {
            p[0] = (int)std::round (p[0]);
            p[1] = (int)std::round (p[1]);
            p[2] = (int)std::round (p[2]);
          }

          p = transform.voxel2scanner.cast<float>() * p;

          Eigen::Vector3f x_dir = projection.screen_to_model_direction (1.0f, 0.0f, projection.depth_of (p));
          x_dir.normalize();
          x_dir = transform.scanner2image.rotation().cast<float>() * x_dir;
          x_dir[0] *= fixel_header.spacing(0);
          x_dir[1] *= fixel_header.spacing(1);
          x_dir[2] *= fixel_header.spacing(2);
          x_dir = transform.image2scanner.rotation().cast<float>() * x_dir;

          Eigen::Vector3f y_dir = projection.screen_to_model_direction (0.0f, 1.0f, projection.depth_of (p));
          y_dir.normalize();
          y_dir = transform.scanner2image.rotation().cast<float>() * y_dir;
          y_dir[0] *= fixel_header.spacing(0);
          y_dir[1] *= fixel_header.spacing(1);
          y_dir[2] *= fixel_header.spacing(2);
          y_dir = transform.image2scanner.rotation().cast<float>() * y_dir;

          Eigen::Vector3f x_width = projection.screen_to_model_direction (projection.width()/2.0f, 0.0f, projection.depth_of (p));
          int nx = std::ceil (x_width.norm() / x_dir.norm());
          Eigen::Vector3f y_width = projection.screen_to_model_direction (0.0f, projection.height()/2.0f, projection.depth_of (p));
          int ny = std::ceil (y_width.norm() / y_dir.norm());

          regular_grid_buffer_pos.clear ();
          regular_grid_buffer_dir.clear ();
          regular_grid_buffer_val.clear ();
          regular_grid_buffer_colour.clear ();
          regular_grid_buffer_threshold.clear ();

          const auto& val_buffer = current_fixel_value_state ().buffer_store;
          const auto& col_buffer = current_fixel_colour_state ().buffer_store;
          const auto& threshold_buffer = current_fixel_threshold_state ().buffer_store;

          for (int y = -ny; y <= ny; ++y) {
            for (int x = -nx; x <= nx; ++x) {
              Eigen::Vector3f scanner_pos = p + float(x)*x_dir + float(y)*y_dir;
              Eigen::Vector3f voxel_pos = transform.scanner2voxel.cast<float>() * scanner_pos;
              std::array<int, 3> voxel {{ (int)std::round (voxel_pos[0]), (int)std::round (voxel_pos[1]), (int)std::round (voxel_pos[2]) }};

              // Find and add point indices that correspond to projected voxel
              const auto &voxel_indices = voxel_to_indices_map[voxel];

              // Load all corresponding fixel data into separate buffer
              // We can't reuse original buffer because off-axis rendering means that
              // two or more points in our regular grid may correspond to the same nearest voxel
              for(const GLsizei index : voxel_indices) {
                regular_grid_buffer_pos.push_back (scanner_pos);
                regular_grid_buffer_dir.push_back (dir_buffer_store[index]);
                if (scale_type == Value)
                  regular_grid_buffer_val.push_back (val_buffer[index]);
                if (colour_type == CValue)
                  regular_grid_buffer_colour.push_back (col_buffer[index]);
                regular_grid_buffer_threshold.push_back (threshold_buffer[index]);
              }
            }
          }

          if(!regular_grid_buffer_pos.size())
            return;

          MRView::GrabContext context;

          regular_grid_vao.bind ();
          regular_grid_vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, regular_grid_buffer_pos.size () * sizeof(Eigen::Vector3f),
                          &regular_grid_buffer_pos[0], gl::DYNAMIC_DRAW);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          // fixel directions
          regular_grid_dir_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, regular_grid_buffer_dir.size () * sizeof(Eigen::Vector3f),
                          &regular_grid_buffer_dir[0], gl::DYNAMIC_DRAW);
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          // fixel values
          if (scale_type == Value) {
            regular_grid_val_buffer.bind (gl::ARRAY_BUFFER);
            gl::BufferData (gl::ARRAY_BUFFER, regular_grid_buffer_val.size () * sizeof(float),
                          &regular_grid_buffer_val[0], gl::DYNAMIC_DRAW);
            gl::EnableVertexAttribArray (2);
            gl::VertexAttribPointer (2, 1, gl::FLOAT, gl::FALSE_, 0, (void*)0);
          }

          // fixel colours
          if (colour_type == CValue) {
            regular_grid_colour_buffer.bind (gl::ARRAY_BUFFER);
            gl::BufferData (gl::ARRAY_BUFFER, regular_grid_buffer_colour.size () * sizeof(float),
                          &regular_grid_buffer_colour[0], gl::DYNAMIC_DRAW);
            gl::EnableVertexAttribArray (3);
            gl::VertexAttribPointer (3, 1, gl::FLOAT, gl::FALSE_, 0, (void*)0);
          }

          // fixel threshold
          regular_grid_threshold_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, regular_grid_buffer_threshold.size () * sizeof(float),
                          &regular_grid_buffer_threshold[0], gl::DYNAMIC_DRAW);
          gl::EnableVertexAttribArray (4);
          gl::VertexAttribPointer (4, 1, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void AbstractFixel::load_image (const std::string& filename)
        {
          // Make sure to set graphics context!
          // We're setting up vertex array objects
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          load_image_buffer ();

          for (auto& fixel_val : fixel_values)
            fixel_val.second.initialise_windowing ();

          set_scale_type_index (0);
          set_threshold_type_index (0);

          size_t colour_index(0);
          const auto initial_col_key = std::find (colour_types.begin (), colour_types.end (), Path::basename (filename));
          if (initial_col_key != colour_types.end ())
            colour_index = std::distance (colour_types.begin (), initial_col_key);

          set_colour_type_index (colour_index);

          regular_grid_buffer_pos = std::vector<Eigen::Vector3f> (pos_buffer_store.size ());

          regular_grid_vao.gen ();

          regular_grid_vertex_buffer.gen ();
          regular_grid_dir_buffer.gen ();
          regular_grid_val_buffer.gen ();
          regular_grid_colour_buffer.gen ();
          regular_grid_threshold_buffer.gen ();

          vertex_array_object.gen ();
          vertex_array_object.bind ();

          vertex_buffer.gen ();
          direction_buffer.gen ();
          value_buffer.gen ();
          colour_buffer.gen ();
          threshold_buffer.gen ();

          // voxel centres
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, pos_buffer_store.size () * sizeof(Eigen::Vector3f), &(pos_buffer_store[0][0]), gl::STATIC_DRAW);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          dir_buffer_dirty = true;
          value_buffer_dirty = true;
          colour_buffer_dirty = true;
          threshold_buffer_dirty = true;
        }


        void AbstractFixel::reload_directions_buffer ()
        {
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          vertex_array_object.bind ();

          // fixel directions
          direction_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, dir_buffer_store.size () * sizeof(Eigen::Vector3f), &(dir_buffer_store)[0][0], gl::STATIC_DRAW);
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void AbstractFixel::reload_values_buffer ()
        {
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          if (scale_type == Unity)
            return;

          const auto& fixel_val = current_fixel_value_state ();
          const auto& val_buffer = fixel_val.buffer_store;

          vertex_array_object.bind ();

          // fixel values
          value_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, val_buffer.size () * sizeof(float), &(val_buffer)[0], gl::STATIC_DRAW);
          gl::EnableVertexAttribArray (2);
          gl::VertexAttribPointer (2, 1, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void AbstractFixel::reload_colours_buffer ()
        {
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          if (colour_type == Direction)
            return;

          const auto& fixel_val = current_fixel_colour_state ();
          const auto& val_buffer = fixel_val.buffer_store;

          vertex_array_object.bind ();

          // fixel colours
          colour_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, val_buffer.size () * sizeof(float), &(val_buffer)[0], gl::STATIC_DRAW);
          gl::EnableVertexAttribArray (3);
          gl::VertexAttribPointer (3, 1, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void AbstractFixel::reload_threshold_buffer ()
        {
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          const auto& fixel_val = current_fixel_threshold_state ();
          const auto& val_buffer = fixel_val.buffer_store;

          vertex_array_object.bind ();

          // fixel colours
          threshold_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, val_buffer.size () * sizeof(float), &(val_buffer)[0], gl::STATIC_DRAW);
          gl::EnableVertexAttribArray (4);
          gl::VertexAttribPointer (4, 1, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }

      }
    }
  }
}
