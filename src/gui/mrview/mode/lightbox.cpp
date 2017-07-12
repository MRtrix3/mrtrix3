/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "gui/mrview/mode/lightbox.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        bool LightBox::show_grid_lines(true);
        bool LightBox::show_volumes(false);
        size_t LightBox::n_rows(3);
        size_t LightBox::n_cols(5);
        size_t LightBox::volume_increment(1);
        float LightBox::slice_focus_increment(1.f);
        float LightBox::slice_focus_inc_adjust_rate(0.2f);
        std::string LightBox::prev_image_name;

        LightBox::LightBox () :
          layout_is_dirty(true),
          current_slice_index((n_rows*n_cols) / 2),
          slices_proj_focusdelta(n_rows*n_cols, proj_focusdelta(projection, 0.f))
        {
          Image* img = image();

          if(!img || prev_image_name != img->header().name())
            image_changed_event();
          else {
            set_volume_increment(1);
            set_slice_increment(slice_focus_increment);
          }
        }


        void LightBox::set_rows(size_t rows)
        {
          n_rows = rows;
          layout_is_dirty = true;
          updateGL();
        }

        void LightBox::set_cols(size_t cols)
        {
          n_cols = cols;
          layout_is_dirty = true;
          updateGL();
        }

        void LightBox::set_volume_increment(size_t vol_inc)
        {
          volume_increment = vol_inc;
          update_volume_indices();
          updateGL();
        }

        void LightBox::set_slice_increment(float inc)
        {
          slice_focus_increment = inc;
          update_slices_focusdelta();
          updateGL();
        }

        void LightBox::set_show_grid(bool show_grid)
        {
          show_grid_lines = show_grid;
          updateGL();
        }

        void LightBox::set_show_volumes(bool show_vol)
        {
          show_volumes = show_vol;
          updateGL();
        }

        inline bool LightBox::render_volumes()
        {
          return show_volumes && image () && image()->image.ndim() == 4;
        }

        inline void LightBox::update_layout()
        {
          // Can't use vector resize() because Projection needs to be assignable
          slices_proj_focusdelta = vector<proj_focusdelta>(
              n_cols * n_rows,
              proj_focusdelta(projection, 0.f));

          set_current_slice_index((n_rows * n_cols) / 2);
          update_slices_focusdelta();

          update_volume_indices();

          frame_VB.clear();
          frame_VAO.clear();
        }

        void LightBox::set_current_slice_index(size_t slice_index)
        {
          int prev_index = current_slice_index;
          current_slice_index = slice_index;

          if (render_volumes()) {
            window().set_image_volume(3, volume_indices[current_slice_index]);
          }

          else if (prev_index != (int)current_slice_index) {
            const Projection& slice_proj = slices_proj_focusdelta[current_slice_index].first;
            float focus_delta = slices_proj_focusdelta[current_slice_index].second;

            const Eigen::Vector3f slice_focus = move_in_out_displacement(focus_delta, slice_proj);
            set_focus(focus() + slice_focus);
            update_slices_focusdelta();
          }
        }

        void LightBox::update_slices_focusdelta()
        {
          const int current_slice_index_int = current_slice_index;
          for(int i = 0, N = slices_proj_focusdelta.size(); i < N; ++i) {
            slices_proj_focusdelta[i].second =
              slice_focus_increment * (i - current_slice_index_int);
          }
        }

        void LightBox::update_volume_indices()
        {
          bool is_4d = image () && image()->image.ndim() == 4;
          volume_indices.resize (n_rows * n_cols, 0);

          if (!is_4d)
            return;

          int n_vols = image()->image.size(3);
          int initial_vol = image()->image.index(3);

          for(int i = 0, N = volume_indices.size(); i < N; ++i)
            volume_indices[i] = std::min(std::max(initial_vol + (int)volume_increment * (i - (int)current_slice_index), 0), n_vols - 1);
        }

        void LightBox::draw_plane_primitive (int axis, Displayable::Shader& shader_program, Projection& with_projection)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (visible)
            image()->render3D (shader_program, with_projection, with_projection.depth_of (focus()));
          render_tools (with_projection, false, axis, slice (axis));
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }

        void LightBox::paint(Projection&)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          // Setup OpenGL environment:
          gl::Disable (gl::BLEND);
          gl::Disable (gl::DEPTH_TEST);
          gl::DepthMask (gl::FALSE_);
          gl::ColorMask (gl::TRUE_, gl::TRUE_, gl::TRUE_, gl::TRUE_);

          GLint x = projection.x_position(), y = projection.y_position();
          GLint w = projection.width(), h = projection.height();
          GLfloat dw = w / (float)n_cols, dh = h / (float)n_rows;

          const Eigen::Vector3f orig_focus = window().focus();

          if (layout_is_dirty) {
            update_layout();
            layout_is_dirty = false;
          }

          bool rend_vols = render_volumes();

          size_t slice_idx = 0;
          for(size_t row = 0; row < n_rows; ++row) {
            for(size_t col = 0; col < n_cols; ++col, ++slice_idx) {
              Projection& slice_proj = slices_proj_focusdelta[slice_idx].first;

              // Place the first slice in the top-left corner
              slice_proj.set_viewport(window(), x + dw * col, y + h - (dh * (row+1)), dw, dh);

              // We need to setup the modelview/proj matrices before we set the new focus
              // because move_in_out_displacement is reliant on MVP
              setup_projection (plane(), slice_proj);

              if (rend_vols)
                image()->image.index(3) = volume_indices[slice_idx];

              else {
                float focus_delta = slices_proj_focusdelta[slice_idx].second;
                Eigen::Vector3f slice_focus = move_in_out_displacement(focus_delta, slice_proj);
                set_focus(orig_focus + slice_focus);
              }

              draw_plane_primitive(plane(), slice_shader, slice_proj);

              if(slice_idx == current_slice_index) {
                // Drawing plane may alter the depth test state
                // so need to ensure that crosshairs/labels will be visible
                gl::Disable (gl::DEPTH_TEST);
                draw_crosshairs(slice_proj);
                draw_orientation_labels(slice_proj);
              }
            }
          }

          // Restore view state
          if(rend_vols)
            image()->image.index(3) = volume_indices[current_slice_index];
          set_focus(orig_focus);
          projection.set_viewport(window(), x, y, w, h);

          // Users may want to screen capture without grid lines
          if(show_grid_lines) {
            gl::Disable(gl::DEPTH_TEST);
            draw_grid();
          }
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }

        void LightBox::draw_grid()
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if(n_cols < 1 && n_rows < 1)
            return;

          size_t num_points = ((n_rows - 1) + (n_cols - 1)) * 4;

          GL::mat4 MV = GL::identity();
          GL::mat4 P = GL::ortho (0, width(), 0, height(), -1.0, 1.0);
          projection.set (MV, P);

          if (!frame_VB || !frame_VAO) {
            frame_VB.gen();
            frame_VAO.gen();

            frame_VB.bind (gl::ARRAY_BUFFER);
            frame_VAO.bind();

            gl::EnableVertexAttribArray (0);
            gl::VertexAttribPointer (0, 2, gl::FLOAT, gl::FALSE_, 0, (void*)0);

            GLfloat data[num_points];

            // Grid line stride
            float x_inc = 2.f / n_cols;
            float y_inc = 2.f / n_rows;

            size_t pt_idx = 0;
            // Row grid lines
            for(size_t row = 1; row < n_rows; ++row, pt_idx += 4) {
              float y_pos = (y_inc * row) - 1.f;
              data[pt_idx] = -1.f;
              data[pt_idx+1] = y_pos;
              data[pt_idx+2] = 1.f;
              data[pt_idx+3] = y_pos;
            }

            // Column grid lines
            for(size_t col = 1; col < n_cols; ++col, pt_idx += 4) {
              float x_pos = (x_inc * col) - 1.f;
              data[pt_idx] = x_pos;
              data[pt_idx+1] = -1.f;
              data[pt_idx+2] = x_pos;
              data[pt_idx+3] = 1.f;
            }

            gl::BufferData (gl::ARRAY_BUFFER, sizeof(data), data, gl::STATIC_DRAW);
          }
          else
            frame_VAO.bind();

          if (!frame_program) {
            GL::Shader::Vertex vertex_shader (
                "layout(location=0) in vec2 pos;\n"
                "void main () {\n"
                "  gl_Position = vec4 (pos, 0.0, 1.0);\n"
                "}\n");
            GL::Shader::Fragment fragment_shader (
                "out vec3 color;\n"
                "void main () {\n"
                "  color = vec3 (0.1);\n"
                "}\n");
            frame_program.attach (vertex_shader);
            frame_program.attach (fragment_shader);
            frame_program.link();
          }

          frame_program.start();
          gl::DrawArrays (gl::LINES, 0, num_points / 2);
          frame_program.stop();
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }

        void LightBox::mouse_press_event()
        {
          GLint x = projection.x_position(), y = projection.y_position();
          GLint w = projection.width(), h = projection.height();
          GLint dw = w / n_cols, dh = h / n_rows;

          const auto& mouse_pos = window().mouse_position();

          size_t col = (mouse_pos.x() - x) / dw;
          size_t row = n_rows - (mouse_pos.y() - y) / dh - 1;

          if(col < n_cols && row < n_rows) {
            set_current_slice_index(slice_index(row, col));
          }
        }

        // Called when we get a mouse move event
        void LightBox::set_focus_event()
        {
          Base::set_focus_event();
          mouse_press_event();
        }

        void LightBox::image_changed_event()
        {
          Base::image_changed_event();

          update_volume_indices();

          if(image()) {
            const auto& header = image()->header();
            if (prev_image_name.empty()) {
              float slice_inc = std::pow (header.spacing(0)*header.spacing(1)*header.spacing(2), 1.f/3.f);
              slice_focus_inc_adjust_rate = slice_inc / 5.f;

              set_slice_increment(slice_inc);
              emit slice_increment_reset();
            }

            prev_image_name = image()->header().name();
          }
          else
            prev_image_name.clear();
        }

      }
    }
  }
}
