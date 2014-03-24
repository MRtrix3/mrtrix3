/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

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

#ifndef __gui_mrview_tool_fixel_h__
#define __gui_mrview_tool_fixel_h__

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

            void draw (const Projection& transform, bool is_3D);
            void drawOverlays (const Projection& transform);
//            bool crop_to_slab () const { return (do_crop_to_slab && not_3D); }
//            bool process_batch_command (const std::string& cmd, const std::string& args);

            QPushButton* hide_all_button;
            float line_thickness;
            bool do_crop_to_grid;
            bool not_3D;
            float line_opacity;
            Model* fixel_list_model;
            QListView* fixel_list_view;


          private slots:
            void fixel_open_slot ();
            void fixel_close_slot ();
            void toggle_shown_slot (const QModelIndex&, const QModelIndex&);
            void hide_all_slot ();
            void on_crop_to_grid_slot (bool is_checked);
            void opacity_slot (int opacity);
            void line_thickness_slot (int thickness);
            void line_size_slot (int thickness);
            void colour_by_direction_slot ();
            void set_colour_slot ();
            void randomise_colour_slot ();
            void selection_changed_slot (const QItemSelection &, const QItemSelection &);
            void show_colour_bar_slot ();
            void select_colourmap_slot (int);
            void colour_changed_slot (int);
            void on_set_scaling_slot ();
            void reset_intensity_slot ();
            void invert_colourmap_slot ();
            void threshold_lower_changed (int unused);
            void threshold_upper_changed (int unused);
            void threshold_lower_value_changed ();
            void threshold_upper_value_changed ();

          protected:
            class Image;

            AdjustButton *slab_entry;
            AdjustButton *max_entry, *min_entry;
            AdjustButton *threshold_lower, *threshold_upper;
            QCheckBox *threshold_upper_box, *threshold_lower_box;
            QComboBox *colour_combobox, *colourmap_combobox;
            QAction *show_colour_bar;
            QAction *invert_scale;
            QMenu *colourmap_menu;
            QAction **colourmap_actions;
            QActionGroup *colourmap_group;
            QToolButton *colourmap_button;

            void updateGL() {
//              window.get_current_mode()->update_overlays = true; TODO
              window.updateGL();
            }

        };
      }
    }
  }
}

#endif




