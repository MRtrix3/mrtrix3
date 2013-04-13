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

#ifndef __gui_mrview_tool_odf_h__
#define __gui_mrview_tool_odf_h__

#include "gui/mrview/tool/base.h"
#include "gui/mrview/adjust_button.h"

class QStringListModel;
class QSlider;
class QCheckBox;
class QColorButton;

namespace MR
{
  namespace GUI
  {
    namespace DWI {
      class RenderFrame;
    }

    namespace MRView
    {
      namespace Tool
      {

        class ODF : public Base
        {
            Q_OBJECT

          public:

            ODF (Window& main_window, Dock* parent);

            void draw2D (const Projection& projection);
            void draw3D (const Projection& projection);


          private slots:
            void onFocusChanged ();
            void image_open_slot ();
            void image_close_slot ();
            // void toggle_shown_slot (const QModelIndex& index);
            void selection_changed_slot (const QItemSelection &, const QItemSelection &);
            void update_slot (int unused);
            void lock_orientation_to_image_slot (int unused);
            void colour_by_direction_slot (int unused);
            void hide_negative_lobes_slot (int unused);
            void use_lighting_slot (int unused);
            void interpolation_slot (int unused);
            void show_axes_slot (int unused);
            void lmax_slot (int value);
            void level_of_detail_slot (int value);
            // void values_changed ();
            // void colourmap_changed (int index);

          protected:
             class Model;
             Model* image_list_model;
             DWI::RenderFrame *render_frame;
             QListView* image_list_view;
             AdjustButton *scale, *overlay_scale;
             QCheckBox *lock_orientation_to_image_box, *hide_negative_lobes_box;
             QCheckBox *colour_by_direction_box, *use_lighting_box, *interpolation_box, *show_axes_box;
             QColorButton *colour_button;
             QSpinBox *lmax_selector, *level_of_detail_selector;
             QGroupBox *overlay_frame;
             QSpinBox *overlay_level_of_detail_selector;
             QComboBox *overlay_grid_selector;
             QCheckBox *overlay_lock_to_grid_box;
 
             void update_selection ();
            virtual void showEvent (QShowEvent* event);
            virtual void closeEvent (QCloseEvent* event);
        };

      }
    }
  }
}

#endif





