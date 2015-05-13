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

#ifndef __gui_mrview_tool_vector_h__
#define __gui_mrview_tool_vector_h__

#include "gui/mrview/tool/base.h"
#include "gui/projection.h"
#include "gui/mrview/adjust_button.h"
#include "gui/mrview/combo_box_error.h"
#include "gui/mrview/colourmap_button.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        class Vector : public Base, public ColourMapButtonObserver
        {
            Q_OBJECT

          public:
            class Model;

            Vector (Window& main_window, Dock* parent);

            virtual ~Vector ();

            void draw (const Projection& transform, bool is_3D, int axis, int slice);
            void drawOverlays (const Projection& transform) override;

            static void add_commandline_options (MR::App::OptionList& options);
            virtual bool process_commandline_option (const MR::App::ParsedOption& opt);

            void selected_colourmap(size_t index, const ColourMapButton&) override;
            void selected_custom_colour(const QColor& colour, const ColourMapButton&) override;
            void toggle_show_colour_bar(bool, const ColourMapButton&) override;
            void toggle_invert_colourmap(bool, const ColourMapButton&) override;
            void reset_colourmap(const ColourMapButton&) override;

            QPushButton* hide_all_button;
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
            void length_multiplier_slot ();
            void length_type_slot (int);
            void selection_changed_slot (const QItemSelection &, const QItemSelection &);
            void colour_changed_slot (int);
            void on_set_scaling_slot ();
            void threshold_lower_changed (int unused);
            void threshold_upper_changed (int unused);
            void threshold_lower_value_changed ();
            void threshold_upper_value_changed ();

          protected:
            ComboBoxWithErrorMsg *colour_combobox;

            QGroupBox *colourmap_option_group;
            QAction *show_colour_bar, *invert_scale;
            ColourMapButton *colourmap_button;

            AdjustButton *min_value, *max_value;
            AdjustButton *threshold_lower, *threshold_upper;
            QCheckBox *threshold_upper_box, *threshold_lower_box;

            ComboBoxWithErrorMsg *length_combobox;
            AdjustButton *length_multiplier;

            QSlider *line_thickness_slider;
            QSlider *opacity_slider;

            QGroupBox *crop_to_slice;

        };
      }
    }
  }
}

#endif




