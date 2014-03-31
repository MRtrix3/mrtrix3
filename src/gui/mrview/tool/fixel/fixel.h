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

#ifndef __gui_mrview_tool_fixel_fixel_h__
#define __gui_mrview_tool_fixel_fixel_h__

#include "gui/mrview/tool/base.h"
#include "gui/projection.h"
#include "gui/mrview/adjust_button.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        class Fixel : public Base
        {
            Q_OBJECT

          public:
            class Model;

            Fixel (Window& main_window, Dock* parent);

            virtual ~Fixel ();

            void draw (const Projection& transform, bool is_3D, int axis, int slice);
            void drawOverlays (const Projection& transform);
            bool process_batch_command (const std::string& cmd, const std::string& args);

            QPushButton* hide_all_button;
            float line_thickness;
            bool do_crop_to_slice;
            bool not_3D;
            float line_opacity;
            Model* fixel_list_model;
            QListView* fixel_list_view;


          private slots:
            void fixel_open_slot ();
            void fixel_close_slot ();
            void toggle_shown_slot (const QModelIndex&, const QModelIndex&);
            void hide_all_slot ();
            void update_selection();
            void on_crop_to_slice_slot (bool is_checked);
            void opacity_slot (int opacity);
            void line_thickness_slot (int thickness);
            void line_length_slot ();
            void on_line_length_by_value_slot (bool length_by_value);
            void selection_changed_slot (const QItemSelection &, const QItemSelection &);
            void show_colour_bar_slot ();
            void select_colourmap_slot ();
            void colour_changed_slot (int);
            void on_set_scaling_slot ();
            void reset_intensity_slot ();
            void invert_colourmap_slot ();
            void threshold_lower_changed (int unused);
            void threshold_upper_changed (int unused);
            void threshold_lower_value_changed ();
            void threshold_upper_value_changed ();

          protected:
            AdjustButton *max_value, *min_value;
            AdjustButton *threshold_lower, *threshold_upper;
            AdjustButton *line_length;
            QGroupBox *line_length_by_value;
            QGroupBox* crop_to_slice;
            QCheckBox *threshold_upper_box, *threshold_lower_box;
            QComboBox *colour_combobox;
            QAction *show_colour_bar;
            QAction *invert_scale;
            QMenu *colourmap_menu;
            QAction **colourmap_actions;
            QActionGroup *colourmap_group;
            QToolButton *colourmap_button;
            QSlider* opacity_slider;
            QSlider* line_thickness_slider;

        };
      }
    }
  }
}

#endif




