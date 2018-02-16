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


#ifndef __gui_mrview_mode_lightbox_h__
#define __gui_mrview_mode_lightbox_h__

#include "gui/mrview/mode/slice.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {
        class LightBox : public Slice
        { MEMALIGN(LightBox)
            Q_OBJECT
            using proj_focusdelta = std::pair<Projection,float>;
          public:
            LightBox();

            void paint(Projection& with_projection) override;
            void mouse_press_event() override;
            void set_focus_event() override;
            void image_changed_event() override;
            const Projection* get_current_projection() const override {
              return &slices_proj_focusdelta[current_slice_index].first; }

            void request_update_mode_gui(ModeGuiVisitor& visitor) const override {
              visitor.update_lightbox_mode_gui(*this); }

            static size_t get_rows() { return n_rows; }
            static size_t get_cols() { return n_cols; }
            static size_t get_volume_increment() { return volume_increment; }
            static float get_slice_increment() { return slice_focus_increment; }
            static float get_slice_inc_adjust_rate() { return slice_focus_inc_adjust_rate; }
            static bool get_show_grid() { return show_grid_lines; }
            static bool get_show_volumes() { return show_volumes; }

            void set_rows(size_t rows);
            void set_cols(size_t cols);
            void set_volume_increment(size_t vol_inc);
            void set_slice_increment(float inc);
            void set_show_grid(bool show_grid);
            void set_show_volumes(bool show_vol);

          public slots:
            void nrows_slot(int value) { set_rows(static_cast<size_t>(value)); }
            void ncolumns_slot(int value) { set_cols(static_cast<size_t>(value));}
            void slice_inc_slot(float value) { set_slice_increment(value); }
            void volume_inc_slot(int value) { set_volume_increment(value); }
            void show_grid_slot (bool value) { set_show_grid(value); }
            void show_volumes_slot (bool value) { set_show_volumes(value); }
            void image_volume_changed_slot() { update_volume_indices(); }

          protected:
            void draw_plane_primitive(int axis, Displayable::Shader& shader_program,
                                      Projection& with_projection) override;

          private:
            static size_t slice_index(size_t row, size_t col) {
              assert(row < n_rows && col < n_cols);
              return (row * n_cols) + col;
            }

            void update_layout();
            void update_volume_indices();
            void update_slices_focusdelta();
            void set_current_slice_index(ssize_t slice_index);
            void draw_grid();
            bool render_volumes();

            // Want layout state to persist even after instance is destroyed
            static bool show_grid_lines, show_volumes;
            static std::string prev_image_name;
            static size_t n_rows, n_cols, volume_increment;
            static float slice_focus_increment;
            static float slice_focus_inc_adjust_rate;

            bool layout_is_dirty;
            static ssize_t current_slice_index;
            vector<ssize_t> volume_indices;
            vector<proj_focusdelta> slices_proj_focusdelta;

            GL::VertexBuffer frame_VB;
            GL::VertexArrayObject frame_VAO;
            GL::Shader::Program frame_program;
          signals:
            void slice_increment_reset();
        };

      }
    }
  }
}


#endif
