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
          header (MR::Header::open (filename)),
          slice_fixel_indices (3),
          slice_fixel_sizes (3),
          slice_fixel_counts (3),
          length_type (Unity),
          fixel_tool (fixel_tool),
          voxel_size_length_multipler (1.f),
          user_line_length_multiplier (1.f),
          line_thickness (0.0015f),
          colour_type (CValue)
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
          vertex_buffer.clear();
          direction_buffer.clear();
          vertex_array_object.clear();
          value_buffer.clear();
          regular_grid_vao.clear();
          regular_grid_vertex_buffer.clear();
          regular_grid_dir_buffer.clear();
          regular_grid_val_buffer.clear();
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

          if (fixel.use_discard_lower())
            source += "uniform float lower;\n";
          if (fixel.use_discard_upper())
            source += "uniform float upper;\n";

          source +=
               "flat out vec3 fColour;\n"
               "void main() {\n";

          if (fixel.use_discard_lower())
            source += "  if (v_fixel_metrics[0].y < lower) return;\n";
          if (fixel.use_discard_upper())
            source += "  if (v_fixel_metrics[0].y > upper) return;\n";

          switch (length_type) {
            case Unity:
              source += "   vec4 line_offset = length_mult * vec4 (v_dir[0], 0);\n";
              break;
            case Amplitude:
              source += "   vec4 line_offset = length_mult * v_fixel_metrics[0].x * vec4 (v_dir[0], 0);\n";
              break;
            case LValue:
              source += "   vec4 line_offset = length_mult * v_fixel_metrics[0].y * vec4 (v_dir[0], 0);\n";
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


        std::string AbstractFixel::Shader::fragment_shader_source (const Displayable&/* fixel*/)
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
          else if (length_type != fixel.length_type)
            return true;
          else if (fixel.internal_buffers_dirty ())
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


        void AbstractFixel::render (const Projection& projection)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          start (fixel_shader);
          projection.set (fixel_shader);

          update_image_buffer ();

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

          regular_grid_buffer_pos.clear();
          regular_grid_buffer_dir.clear();
          regular_grid_buffer_val.clear();

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
                regular_grid_buffer_dir.push_back ((*buffer_dir)[index]);
                regular_grid_buffer_val.push_back ((*buffer_val)[2 * index]);
                regular_grid_buffer_val.push_back ((*buffer_val)[(2 * index) + 1]);
              }
            }
          }

          if(!regular_grid_buffer_pos.size())
            return;

          MRView::GrabContext context;

          regular_grid_vao.bind();
          regular_grid_vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, regular_grid_buffer_pos.size() * sizeof(Eigen::Vector3f),
                          &regular_grid_buffer_pos[0], gl::DYNAMIC_DRAW);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          // fixel directions
          regular_grid_dir_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, regular_grid_buffer_dir.size() * sizeof(Eigen::Vector3f),
                          &regular_grid_buffer_dir[0], gl::DYNAMIC_DRAW);
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          // fixel sizes and values
          regular_grid_val_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, regular_grid_buffer_val.size() * sizeof(float),
                          &regular_grid_buffer_val[0], gl::DYNAMIC_DRAW);
          gl::EnableVertexAttribArray (2);
          gl::VertexAttribPointer (2, 2, gl::FLOAT, gl::FALSE_, 0, (void*)0);
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void AbstractFixel::load_image ()
        {
          // Make sure to set graphics context!
          // We're setting up vertex array objects
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          load_image_buffer ();

          regular_grid_buffer_pos = std::vector<Eigen::Vector3f> (buffer_pos->size ());

          regular_grid_vao.gen ();

          regular_grid_vertex_buffer.gen ();
          regular_grid_dir_buffer.gen ();
          regular_grid_val_buffer.gen ();

          vertex_array_object.gen ();
          vertex_array_object.bind ();

          vertex_buffer.gen ();
          direction_buffer.gen ();
          value_buffer.gen ();

          reload_dir_and_value_buffers ();

          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void AbstractFixel::reload_dir_and_value_buffers ()
        {
          MRView::GrabContext context;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;

          vertex_array_object.bind ();

          // voxel centres
          vertex_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, buffer_pos->size () * sizeof(Eigen::Vector3f), &(*buffer_pos)[0][0], gl::STATIC_DRAW);
          gl::EnableVertexAttribArray (0);
          gl::VertexAttribPointer (0, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          // fixel directions
          direction_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, buffer_dir->size () * sizeof(Eigen::Vector3f), &(*buffer_dir)[0][0], gl::STATIC_DRAW);
          gl::EnableVertexAttribArray (1);
          gl::VertexAttribPointer (1, 3, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          // fixel sizes and values
          value_buffer.bind (gl::ARRAY_BUFFER);
          gl::BufferData (gl::ARRAY_BUFFER, buffer_val->size () * sizeof(float), &(*buffer_val)[0], gl::STATIC_DRAW);
          gl::EnableVertexAttribArray (2);
          gl::VertexAttribPointer (2, 2, gl::FLOAT, gl::FALSE_, 0, (void*)0);

          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void Fixel::load_image_buffer()
        {
          for (size_t axis = 0; axis < 3; ++axis) {
            const size_t axis_size = fixel_data->size (axis);
            slice_fixel_indices[axis].resize (axis_size);
            slice_fixel_sizes  [axis].resize (axis_size);
            slice_fixel_counts [axis].resize (axis_size, 0);
          }

          for (auto l = Loop(*fixel_data) (*fixel_data); l; ++l) {

            const std::array<int, 3> voxel {{ int(fixel_data->index(0)), int(fixel_data->index(1)), int(fixel_data->index(2)) }};
            Eigen::Vector3f pos { float(voxel[0]), float(voxel[1]), float(voxel[2]) };
            pos = transform.voxel2scanner.cast<float>() * pos;

            for (size_t f = 0; f != fixel_data->value().size(); ++f) {

              if (fixel_data->value()[f].value > value_max)
                value_max = fixel_data->value()[f].value;
              if (fixel_data->value()[f].value < value_min)
                value_min = fixel_data->value()[f].value;

              buffer_pos->push_back (pos);
              buffer_dir->push_back (fixel_data->value()[f].dir);
              buffer_val->push_back (fixel_data->value()[f].size);
              buffer_val->push_back (fixel_data->value()[f].value);

              GLint point_index = buffer_pos->size() - 1;

              for (size_t axis = 0; axis < 3; ++axis) {
                slice_fixel_indices[axis][voxel[axis]].push_back (point_index);
                slice_fixel_sizes  [axis][voxel[axis]].push_back (1);
                slice_fixel_counts [axis][voxel[axis]]++;
              }

              voxel_to_indices_map[voxel].push_back (point_index);
            }
          }

          this->set_windowing (value_min, value_max);
          greaterthan = value_max;
          lessthan = value_min;
        }


        void PackedFixel::load_image_buffer()
        {
          size_t ndim = fixel_data->ndim();

          if (ndim != 4)
            throw InvalidImageException ("Vector image " + filename
                                         + " should contain 4 dimensions. Instead "
                                         + str(ndim) + " found.");

          size_t dim4_len = fixel_data->size (3);

          if (dim4_len % 3)
            throw InvalidImageException ("Expecting 4th-dimension size of vector image "
                                         + filename + " to be a multiple of 3. Instead "
                                         + str(dim4_len) + " entries found.");

          for (size_t axis = 0; axis < 3; ++axis) {
            slice_fixel_indices[axis].resize (fixel_data->size (axis));
            slice_fixel_sizes  [axis].resize (fixel_data->size (axis));
            slice_fixel_counts [axis].resize (fixel_data->size (axis), 0);
          }

          const size_t n_fixels = dim4_len / 3;

          for (auto l = Loop(*fixel_data) (*fixel_data); l; ++l) {

            const std::array<int, 3> voxel {{ int(fixel_data->index(0)), int(fixel_data->index(1)), int(fixel_data->index(2)) }};
            Eigen::Vector3f pos { float(voxel[0]), float(voxel[1]), float(voxel[2]) };
            pos = transform.voxel2scanner.cast<float>() * pos;

            for (size_t f = 0; f < n_fixels; ++f) {

              // Fetch the vector components
              Eigen::Vector3f vector;
              fixel_data->index(3) = 3*f;
              vector[0] = fixel_data->value();
              fixel_data->index(3)++;
              vector[1] = fixel_data->value();
              fixel_data->index(3)++;
              vector[2] = fixel_data->value();

              const float length = vector.norm();
              value_min = std::min (value_min, length);
              value_max = std::max (value_max, length);

              buffer_pos->push_back (pos);
              buffer_dir->push_back (vector.normalized());

              // Use the vector length to represent both fixel amplitude and value
              buffer_val->push_back (length);
              buffer_val->push_back (length);

              GLint point_index = buffer_pos->size() - 1;

              for (size_t axis = 0; axis < 3; ++axis) {
                slice_fixel_indices[axis][voxel[axis]].push_back (point_index);
                slice_fixel_sizes  [axis][voxel[axis]].push_back (1);
                slice_fixel_counts [axis][voxel[axis]]++;
              }

              voxel_to_indices_map[voxel].push_back (point_index);
            }
          }

          this->set_windowing (value_min, value_max);
          greaterthan = value_max;
          lessthan = value_min;
        }


        void FixelFolder::load_image_buffer()
        {
          for (size_t axis = 0; axis < 3; ++axis) {
            slice_fixel_indices[axis].resize (fixel_data->size (axis));
            slice_fixel_sizes  [axis].resize (fixel_data->size (axis));
            slice_fixel_counts [axis].resize (fixel_data->size (axis), 0);
          }

          // Load fixel index image
          for (auto l = Loop(0, 3) (*fixel_data); l; ++l) {

            const std::array<int, 3> voxel {{ int(fixel_data->index(0)), int(fixel_data->index(1)), int(fixel_data->index(2)) }};
            Eigen::Vector3f pos { float(voxel[0]), float(voxel[1]), float(voxel[2]) };
            pos = transform.voxel2scanner.cast<float>() * pos;

            fixel_data->index (3) = 0;
            const size_t nfixels = fixel_data->value ();

            for (size_t f = 0; f < nfixels; ++f) {

              buffer_pos->push_back (pos);

              const GLint point_index = buffer_pos->size () - 1;

              for (size_t axis = 0; axis < 3; ++axis) {
                slice_fixel_indices[axis][voxel[axis]].push_back (point_index);
                slice_fixel_sizes  [axis][voxel[axis]].push_back (1);
                slice_fixel_counts [axis][voxel[axis]]++;
              }

              voxel_to_indices_map[voxel].push_back (point_index);
            }
          }

          auto data_headers = FixelFormat::find_data_headers (Path::dirname (fixel_data->name ()), *fixel_data);

          // Load fixel data direction images
          for (Header& header : data_headers) {

            if (header.size (1) != 3) continue;

            auto data_image = header.get_image<float> ().with_direct_io ();
            const auto data_key = Path::basename (data_image.name ());
            buffer_dir_dict[data_key];

            data_image.index (1) = 0;
            for (auto l = Loop(0, 3) (*fixel_data); l; ++l) {
              fixel_data->index (3) = 0;
              const size_t nfixels = fixel_data->value ();
              fixel_data->index (3) = 1;
              const size_t offset = fixel_data->value ();
              for (size_t f = 0; f < nfixels; ++f) {
                data_image.index (0) = offset + f;
                buffer_dir_dict[data_key].emplace_back (data_image.row (1));
              }
            }
          }

          if (!buffer_dir_dict.size())
            throw InvalidImageException ("Fixel index image " + fixel_data->name () + " has no associated directions file");

          // Load fixel data value images
          for (auto& header : data_headers) {

            if (header.size (1) != 1) continue;

            auto data_image = header.get_image<float> ();
            const auto data_key = Path::basename (header.name ());
            buffer_val_dict[data_key];
            std::pair<float, float> min_max = { std::numeric_limits<float>::max (), std::numeric_limits<float>::min () };

            value_types.push_back (data_key);

            data_image.index (1) = 0;
            for (auto l = Loop(0, 3) (*fixel_data); l; ++l) {
              fixel_data->index (3) = 0;
              const size_t nfixels = fixel_data->value ();
              fixel_data->index (3) = 1;
              const size_t offset = fixel_data->value ();

              for (size_t f = 0; f < nfixels; ++f) {
                data_image.index (0) = offset + f;
                float value = data_image.value ();
                buffer_val_dict[data_key].emplace_back (value);
                // FIXME: Shader needs two values atm
                buffer_val_dict[data_key].emplace_back (value);
                min_max = { std::min (min_max.first, value), std::max (min_max.second, value) };
              }
            }

            buffer_min_max_dict[data_key] = min_max;
          }

          if (!buffer_val_dict.size())
            throw InvalidImageException ("Fixel index image " + fixel_data->name () + " has no associated value image files");

          c_buffer_dir = buffer_dir_dict.begin()->first;
          c_buffer_val = buffer_val_dict.begin()->first;

          buffer_dirty = true;
          update_image_buffer ();
        }


        void FixelFolder::update_image_buffer () {

          if (buffer_dirty) {
            buffer_dir = (&buffer_dir_dict[c_buffer_dir]);
            buffer_val = (&buffer_val_dict[c_buffer_val]);

            reload_dir_and_value_buffers ();

            buffer_dirty = false;

            std::tie(value_min, value_max) = buffer_min_max_dict[c_buffer_val];
            this->set_windowing (value_min, value_max);
            greaterthan = value_max;
            lessthan = value_min;
          }
        }


        void FixelFolder::set_length_type (FixelLengthType value) {

          if (value != FixelLengthType::Unity) {

            size_t value_index = (size_t)value;

            if (value_index < value_types.size()) {
              c_buffer_val = value_types[value_index];
              buffer_dirty = true;
              std::tie(value_min, value_max) = buffer_min_max_dict[c_buffer_val];
              this->set_windowing (value_min, value_max);
              greaterthan = value_max;
              lessthan = value_min;
            }

            value = Amplitude;
          }

          length_type = value;
        }

      }
    }
  }
}
