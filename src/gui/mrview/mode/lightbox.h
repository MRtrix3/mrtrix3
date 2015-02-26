/*
   Copyright 2015 Brain Research Institute, Melbourne, Australia

   Written by Rami Tabbara, 18/02/15.

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
{
    Q_OBJECT
    using proj_focusdelta = std::pair<Projection,float>;
public:
    LightBox(Window &parent);

    void paint(Projection& with_projection) override;
    void mouse_press_event() override;
    void set_focus_event() override;
    void image_changed_event() override;
    const Projection* get_current_projection() const override {
        return &slices_proj_focusdelta[current_slice_index].first; }

    static size_t get_rows() { return n_rows; }
    static size_t get_cols() { return n_cols; }
    static float get_slice_increment() { return slice_focus_increment; }
    static bool get_show_grid() { return show_grid_lines; }

    void set_rows(size_t rows);
    void set_cols(size_t cols);
    void set_slice_increment(float inc);
    void set_show_grid(bool show_grid);
private:
    static size_t slice_index(size_t row, size_t col) {
        assert(row < n_rows && col < n_cols);
        return (row * n_cols) + col;
    }

    void update_layout();
    void update_slices_focusdelta();
    void set_current_slice_index(size_t slice_index);
    void draw_grid();

    // Want layout state to persist even after instance is destroyed
    static bool show_grid_lines;
    static size_t n_rows, n_cols;
    static float slice_focus_increment;

    bool layout_is_dirty;
    size_t current_slice_index;
    std::vector<proj_focusdelta> slices_proj_focusdelta;

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
