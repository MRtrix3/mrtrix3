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

#ifndef __gui_mrview_tool_view_h__
#define __gui_mrview_tool_view_h__

#include "gui/mrview/tool/base.h"

namespace MR
{
  namespace GUI
  {

    namespace MRView
    {
      class AdjustButton;

      namespace Tool
      {


        class ClipPlane 
        {
          public:
            GL::vec4 plane;
            bool active;
            std::string name;
        };

        class View : public Base
        {
          Q_OBJECT
          public:
            View (Window& main_window, Dock* parent);

            QPushButton *clip_on_button[3], *clip_edit_button[3], *clip_modify_button;

            std::vector< std::pair<GL::vec4,bool> > get_active_clip_planes () const;
            std::vector<GL::vec4*> get_clip_planes_to_be_edited () const;

          protected:
            virtual void showEvent (QShowEvent* event);
            virtual void closeEvent (QCloseEvent* event);

          private slots:
            void onImageChanged ();
            void onFocusChanged ();
            void onFOVChanged ();
            void onSetFocus ();
            void onPlaneChanged ();
            void onSetPlane (int index);
            void onSetScaling ();
            void onScalingChanged ();
            void onSetTransparency ();
            void onSetFOV ();
            void onCheckThreshold (bool);
            void onModeChanged ();
            void clip_planes_right_click_menu_slot (const QPoint& pos);
            void clip_planes_selection_changed_slot ();
            void clip_planes_toggle_shown_slot();

            void clip_planes_add_axial_slot ();
            void clip_planes_add_sagittal_slot ();
            void clip_planes_add_coronal_slot ();

            void clip_planes_reset_axial_slot ();
            void clip_planes_reset_sagittal_slot ();
            void clip_planes_reset_coronal_slot ();

            void clip_planes_invert_slot ();
            void clip_planes_remove_slot ();
            void clip_planes_clear_slot ();

            void light_box_rows_slot (int value);
            void light_box_columns_slot (int value);
            void light_box_slice_inc_slot ();
            void light_box_show_grid_slot (bool value);
            void light_box_slice_inc_reset_slot ();

          private:
            AdjustButton *focus_x, *focus_y, *focus_z; 
            QSpinBox **voxel_pos;
            AdjustButton *max_entry, *min_entry, *fov;
            AdjustButton *transparent_intensity, *opaque_intensity;
            AdjustButton *lower_threshold, *upper_threshold;
            QCheckBox *lower_threshold_check_box, *upper_threshold_check_box;
            QComboBox *plane_combobox;
            QGroupBox *transparency_box, *threshold_box, *clip_box, *lightbox_box;
            QSlider *opacity;
            QMenu *clip_planes_option_menu, *clip_planes_reset_submenu;
            QAction *clip_planes_new_axial_action, *clip_planes_new_sagittal_action, *clip_planes_new_coronal_action;
            QAction *clip_planes_reset_axial_action, *clip_planes_reset_sagittal_action, *clip_planes_reset_coronal_action;
            QAction *clip_planes_invert_action, *clip_planes_remove_action, *clip_planes_clear_action;
            AdjustButton* light_box_slice_inc;
            QSpinBox *light_box_rows, *light_box_cols;
            QCheckBox *light_box_show_grid;

            class ClipPlaneModel;
            ClipPlaneModel* clip_planes_model;
            QListView* clip_planes_list_view;

            void connect_mode_specific_slots ();
            void init_lightbox_gui (QLayout* parent);
            void reset_light_box_gui_controls ();
            void set_transparency_from_image ();

        };

      }
    }
  }
}

#endif





