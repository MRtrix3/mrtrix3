/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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

        bool LightBox::show_grid_lines = true;
        bool LightBox::show_volumes = false;
        ssize_t LightBox::n_rows = 3;
        ssize_t LightBox::n_cols = 5;
        ssize_t LightBox::volume_increment = 1;
        float LightBox::slice_focus_increment = 1.0f;
        float LightBox::slice_focus_inc_adjust_rate = 0.2f;
        std::string LightBox::prev_image_name;
        ssize_t LightBox::current_slice_index = 0;






        LightBox::LightBox ()
        {
          Image* img = image();

          if(!img || prev_image_name != img->header().name())
            image_changed_event();
          else {
            set_volume_increment (volume_increment);
            set_slice_increment (slice_focus_increment);
          }
        }




        void LightBox::set_rows (size_t rows)
        {
          n_rows = rows;
          frame_VB.clear();
          frame_VAO.clear();
          updateGL();
        }




        void LightBox::set_cols (size_t cols)
        {
          n_cols = cols;
          frame_VB.clear();
          frame_VAO.clear();
          updateGL();
        }





        void LightBox::set_volume_increment (size_t vol_inc)
        {
          volume_increment = vol_inc;
          updateGL();
        }





        void LightBox::set_slice_increment (float inc)
        {
          slice_focus_increment = inc;
          updateGL();
        }





        void LightBox::set_show_grid (bool show_grid)
        {
          show_grid_lines = show_grid;
          updateGL();
        }





        void LightBox::set_show_volumes (bool show_vol)
        {
          show_volumes = show_vol;
          updateGL();
        }





        inline bool LightBox::render_volumes()
        {
          return show_volumes && image () && image()->image.ndim() == 4;
        }






        void LightBox::draw_plane_primitive (int axis, Displayable::Shader& shader_program, Projection& with_projection)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          // Setup OpenGL environment:
          gl::Disable (gl::BLEND);
          gl::Disable (gl::DEPTH_TEST);
          gl::DepthMask (gl::FALSE_);
          gl::ColorMask (gl::TRUE_, gl::TRUE_, gl::TRUE_, gl::TRUE_);

          if (visible)
            image()->render3D (shader_program, with_projection, with_projection.depth_of (focus()));

          render_tools (with_projection, false, axis, slice (axis));
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }






        void LightBox::paint (Projection&)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          GLint x = projection.x_position(), y = projection.y_position();
          GLint w = projection.width(), h = projection.height();
          GLfloat dw = w / (float)n_cols, dh = h / (float)n_rows;

          const Eigen::Vector3f orig_focus = window().focus();
          const ssize_t original_slice_index = image()->image.index(3);

          if (render_volumes()) {
            if (original_slice_index + volume_increment * (n_rows*n_cols-1 - current_slice_index) >= image()->image.size(3))
              current_slice_index = n_rows*n_cols-1-(image()->image.size(3)-1-original_slice_index)/volume_increment;
            if (original_slice_index < volume_increment * current_slice_index)
              current_slice_index = original_slice_index / volume_increment;
          }

          ssize_t slice_idx = 0;
          for (ssize_t row = 0; row < n_rows; ++row) {
            for (ssize_t col = 0; col < n_cols; ++col, ++slice_idx) {

              bool render_plane = true;

              // Place the first slice in the top-left corner
              projection.set_viewport (window(), x + dw * col, y + h - (dh * (row+1)), dw, dh);

              // We need to setup the modelview/proj matrices before we set the new focus
              // because move_in_out_displacement is reliant on MVP
              setup_projection (plane(), projection);

              if (render_volumes()) {
                int vol_index = original_slice_index + volume_increment * (slice_idx - current_slice_index);
                if (vol_index >= 0 && vol_index < image()->image.size(3))
                  image()->image.index(3) = vol_index;
                else
                  render_plane = false;
              }
              else {
                float focus_delta = slice_focus_increment * (slice_idx - current_slice_index);
                Eigen::Vector3f slice_focus = move_in_out_displacement(focus_delta, projection);
                set_focus (orig_focus + slice_focus);
              }
              if (render_plane)
                draw_plane_primitive (plane(), slice_shader, projection);

              if (slice_idx == current_slice_index) {
                // Drawing plane may alter the depth test state
                // so need to ensure that crosshairs/labels will be visible
                gl::Disable (gl::DEPTH_TEST);
                draw_crosshairs (projection);
                draw_orientation_labels (projection);
              }
            }
          }

          // Restore view state
          if (render_volumes())
            image()->image.index(3) = original_slice_index;

          set_focus (orig_focus);
          projection.set_viewport (window(), x, y, w, h);

          // Users may want to screen capture without grid lines
          if (show_grid_lines) {
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
            for (ssize_t row = 1; row < n_rows; ++row, pt_idx += 4) {
              float y_pos = (y_inc * row) - 1.f;
              data[pt_idx] = -1.f;
              data[pt_idx+1] = y_pos;
              data[pt_idx+2] = 1.f;
              data[pt_idx+3] = y_pos;
            }

            // Column grid lines
            for (ssize_t col = 1; col < n_cols; ++col, pt_idx += 4) {
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



        ModelViewProjection LightBox::get_projection_at (int row, int col) const
        {
          GLint x = projection.x_position();
          GLint y = projection.y_position();
          GLint dw = projection.width() / n_cols;
          GLint dh = projection.height() / n_rows;

          ModelViewProjection proj;
          proj.set_viewport (x + dw * col, y + projection.height() - (dh * (row+1)), dw, dh);
          setup_projection (plane(), proj);

          return proj;
        }



        void LightBox::set_focus_event()
        {
          const auto& mouse_pos = window().mouse_position();

          GLint dw = projection.width() / n_cols;
          GLint dh = projection.height() / n_rows;
          ssize_t col = (mouse_pos.x() - projection.x_position()) / dw;
          ssize_t row = n_rows - 1 - (mouse_pos.y() - projection.y_position()) / dh;

          ssize_t prev_index = current_slice_index;
          current_slice_index = col + row*n_cols;

          ModelViewProjection proj = get_projection_at (row, col);

          Eigen::Vector3f slice_focus = focus();
          if (render_volumes()) {
            ssize_t vol = image()->image.index(3) - prev_index + current_slice_index;
            if (vol < 0)
              vol = 0;
            if (vol >= image()->image.size(3))
              vol = image()->image.size(3)-1;
            window().set_image_volume (3, vol);
            emit window().volumeChanged (vol);
          }
          else {
            float focus_delta = slice_focus_increment * (current_slice_index - prev_index);
            slice_focus += move_in_out_displacement(focus_delta, proj);
          }
          set_focus (proj.screen_to_model (mouse_pos, slice_focus));

          updateGL();
        }


        void LightBox::slice_move_event (float x)
        {
          int row = current_slice_index / n_cols;
          int col = current_slice_index - row*n_cols;
          ModelViewProjection proj = get_projection_at (row, col);
          Slice::slice_move_event (proj, x);
        }



        void LightBox::pan_event ()
        {
          int row = current_slice_index / n_cols;
          int col = current_slice_index - row*n_cols;
          ModelViewProjection proj = get_projection_at (row, col);
          Slice::pan_event (proj);
        }


        void LightBox::panthrough_event ()
        {
          int row = current_slice_index / n_cols;
          int col = current_slice_index - row*n_cols;
          ModelViewProjection proj = get_projection_at (row, col);
          Slice::panthrough_event (proj);
        }


        void LightBox::tilt_event ()
        {
          int row = current_slice_index / n_cols;
          int col = current_slice_index - row*n_cols;
          ModelViewProjection proj = get_projection_at (row, col);
          Slice::tilt_event (proj);
        }


        void LightBox::rotate_event ()
        {
          int row = current_slice_index / n_cols;
          int col = current_slice_index - row*n_cols;
          ModelViewProjection proj = get_projection_at (row, col);
          Slice::rotate_event (proj);
        }







        void LightBox::image_changed_event()
        {
          Base::image_changed_event();

          if (image()) {
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
