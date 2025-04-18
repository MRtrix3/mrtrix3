/* Copyright (c) 2008-2025 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __gui_mrview_tool_view_h__
#define __gui_mrview_tool_view_h__

#include "gui/mrview/tool/base.h"
#include "gui/mrview/mode/base.h"
#include "gui/mrview/spin_box.h"
#include "gui/opengl/transformation.h"

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
        { MEMALIGN (ClipPlane)
          public:
            GL::vec4 plane;
            bool active;
            std::string name;
        };

        class View : public Base, public Mode::ModeGuiVisitor, public Tool::CameraInteractor
        { MEMALIGN(View)
          Q_OBJECT
          public:
            View (Dock* parent);

            QPushButton *clip_on_button[3], *clip_edit_button[3], *clip_modify_button;

            vector< std::pair<GL::vec4,bool> > get_active_clip_planes () const;
            vector<GL::vec4*> get_clip_planes_to_be_edited () const;
            bool get_cliphighlightstate () const;
            bool get_clipintersectionmodestate () const;

            void update_lightbox_mode_gui(const Mode::LightBox &mode) override;
            void update_ortho_mode_gui (const Mode::Ortho &mode) override;
            void deactivate () override;
            bool slice_move_event (const ModelViewProjection& projection, float inc) override;
            bool pan_event (const ModelViewProjection& projection) override;
            bool panthrough_event (const ModelViewProjection& projection) override;
            bool tilt_event (const ModelViewProjection& projection) override;
            bool rotate_event (const ModelViewProjection& projection) override;

          protected:
            virtual void showEvent (QShowEvent* event) override;
            virtual void closeEvent (QCloseEvent* event) override;

          private slots:
            void onImageChanged ();
            void onImageVisibilityChanged (bool);
            void onFocusChanged ();
            void onVolumeIndexChanged();
            void onFOVChanged ();
            void onSetFocus ();
            void onSetVoxel ();
            void onSetVolumeIndex ();
            void onPlaneChanged ();
            void onSetPlane (int index);
            void onSetScaling ();
            void onScalingChanged ();
            void onSetTransparency ();
            void onSetFOV ();
            void onCheckThreshold (bool);
            void onModeChanged ();
            void hide_image_slot (bool flag);
            void copy_focus_slot ();
            void copy_voxel_slot ();
            void clip_planes_right_click_menu_slot (const QPoint& pos);
            void clip_planes_selection_changed_slot ();
            void clip_planes_toggle_shown_slot();
            void clip_planes_toggle_highlight_slot();
            void clip_planes_toggle_intersectionmode_slot();

            void clip_planes_add_axial_slot ();
            void clip_planes_add_sagittal_slot ();
            void clip_planes_add_coronal_slot ();

            void clip_planes_reset_axial_slot ();
            void clip_planes_reset_sagittal_slot ();
            void clip_planes_reset_coronal_slot ();

            void clip_planes_invert_slot ();
            void clip_planes_remove_slot ();
            void clip_planes_clear_slot ();

            void light_box_slice_inc_reset_slot ();
            void light_box_toggle_volumes_slot (bool);

          private:
            QPushButton *hide_button;
            QPushButton *copy_focus_button;
            QPushButton *copy_voxel_button;
            AdjustButton *focus_x, *focus_y, *focus_z;
            AdjustButton *voxel_x, *voxel_y, *voxel_z;
            AdjustButton *max_entry, *min_entry, *fov;
            AdjustButton *transparent_intensity, *opaque_intensity;
            AdjustButton *lower_threshold, *upper_threshold;
            QCheckBox *lower_threshold_check_box, *upper_threshold_check_box, *clip_highlight_check_box, *clip_intersectionmode_check_box, *ortho_view_in_row_check_box;
            QComboBox *plane_combobox;
            QGroupBox *volume_box, *transparency_box, *threshold_box, *clip_box, *lightbox_box;
            QSlider *opacity;
            QMenu *clip_planes_option_menu, *clip_planes_reset_submenu;
            QAction *clip_planes_new_axial_action, *clip_planes_new_sagittal_action, *clip_planes_new_coronal_action;
            QAction *clip_planes_reset_axial_action, *clip_planes_reset_sagittal_action, *clip_planes_reset_coronal_action;
            QAction *clip_planes_invert_action, *clip_planes_remove_action, *clip_planes_clear_action;
            GridLayout *volume_index_layout;

            QLabel *light_box_slice_inc_label, *light_box_volume_inc_label;
            AdjustButton *light_box_slice_inc;
            SpinBox *light_box_rows, *light_box_cols, *light_box_volume_inc;
            QCheckBox *light_box_show_grid, *light_box_show_4d;

            class ClipPlaneModel;
            ClipPlaneModel* clip_planes_model;
            QListView* clip_planes_list_view;

            void connect_mode_specific_slots ();
            void init_lightbox_gui (QLayout* parent);
            void reset_light_box_gui_controls ();
            void set_transparency_from_image ();

            void move_clip_planes_in_out (const ModelViewProjection& projection, vector<GL::vec4*>& clip, float distance);
            void rotate_clip_planes (vector<GL::vec4*>& clip, const Eigen::Quaternionf& rot);
        };

      }
    }
  }
}

#endif





