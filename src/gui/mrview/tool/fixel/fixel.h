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

#ifndef __gui_mrview_tool_fixel_h__
#define __gui_mrview_tool_fixel_h__

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
        class Fixel : public Base, public ColourMapButtonObserver, public DisplayableVisitor
        { MEMALIGN (Fixel)
            Q_OBJECT

          public:
            class Model;

            Fixel (Dock* parent);

            virtual ~Fixel () {}

            void draw (const Projection& transform, bool is_3D, int, int) override;
            void draw_colourbars () override;
            size_t visible_number_colourbars () override;
            void render_fixel_colourbar (const Tool::BaseFixel& fixel) override;

            static void add_commandline_options (MR::App::OptionList& options);
            virtual bool process_commandline_option (const MR::App::ParsedOption& opt) override;

            void selected_colourmap (size_t index, const ColourMapButton&) override;
            void selected_custom_colour (const QColor& colour, const ColourMapButton&) override;
            void toggle_show_colour_bar (bool, const ColourMapButton&) override;
            void toggle_invert_colourmap (bool, const ColourMapButton&) override;
            void reset_colourmap (const ColourMapButton&) override;

            QPushButton* hide_all_button;
            bool do_lock_to_grid, do_crop_to_slice;
            bool not_3D;
            float line_opacity;
            Model* fixel_list_model;
            QListView* fixel_list_view;


          private slots:
            void fixel_open_slot ();
            void fixel_close_slot ();
            void toggle_shown_slot (const QModelIndex&, const QModelIndex&);
            void hide_all_slot ();
            void on_lock_to_grid_slot (bool is_checked);
            void on_crop_to_slice_slot (bool is_checked);
            void opacity_slot (int opacity);
            void line_thickness_slot (int thickness);
            void length_multiplier_slot ();
            void length_type_slot (int);
            void threshold_type_slot (int);
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
            ComboBoxWithErrorMsg *threshold_combobox;
            AdjustButton *length_multiplier;

            QSlider *line_thickness_slider;
            QSlider *opacity_slider;

            QGroupBox *lock_to_grid, *crop_to_slice;

            void add_images (vector<std::string>& list);
            void dropEvent (QDropEvent* event) override;

          private:
            void update_gui_controls ();
            void update_gui_scaling_controls (bool reload_scaling_types = true);
            void update_gui_colour_controls (bool reload_colour_types = true);
            void update_gui_threshold_controls (bool reload_threshold_types = true);
        };
      }
    }
  }
}

#endif




