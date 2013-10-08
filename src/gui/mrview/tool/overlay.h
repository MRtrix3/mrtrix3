/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#ifndef __gui_mrview_tool_overlay_h__
#define __gui_mrview_tool_overlay_h__

#include "gui/mrview/tool/base.h"
#include "gui/mrview/adjust_button.h"

class QStringListModel;
class QSlider;
class QCheckBox;

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class Overlay : public Base
        {
            Q_OBJECT

          public:

            Overlay (Window& main_window, Dock* parent);

            void draw2D (const Projection& projection);
            void draw3D (const Projection& projection);

          private slots:
            void image_open_slot ();
            void image_close_slot ();
            void hide_all_slot ();
            void toggle_shown_slot (const QModelIndex&, const QModelIndex&);
            void selection_changed_slot (const QItemSelection &, const QItemSelection &);
            void update_slot (int unused);
            void values_changed ();
            void colourmap_changed (int index);
            void threshold_upper_changed (int unused);
            void threshold_lower_changed (int unused);
            void threshold_upper_value_changed ();
            void threshold_lower_value_changed ();
            void interpolate_changed ();

          protected:
             class Model;
             QPushButton* hide_all_button;
             Model* image_list_model;
             QListView* image_list_view;
             QComboBox* colourmap_combobox;
             AdjustButton *min_value, *max_value, *threshold_lower, *threshold_upper;
             QCheckBox *threshold_lower_box, *threshold_upper_box;
             QCheckBox* interpolate_check_box;
             QSlider *opacity;

             void update_selection ();
        };

      }
    }
  }
}

#endif




