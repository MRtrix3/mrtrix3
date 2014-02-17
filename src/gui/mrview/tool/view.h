/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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

          private:
            AdjustButton *focus_x, *focus_y, *focus_z; 
            QSpinBox **voxel_pos;
            AdjustButton *max_entry, *min_entry, *fov;
            AdjustButton *transparent_intensity, *opaque_intensity;
            AdjustButton *lower_threshold, *upper_threshold;
            QCheckBox *lower_threshold_check_box, *upper_threshold_check_box;
            QComboBox *plane_combobox;
            QGroupBox *transparency_box, *threshold_box, *clip_box;
            QSlider *opacity;
            QMenu *clip_planes_option_menu, *clip_planes_reset_submenu;
            QAction *clip_planes_new_axial_action, *clip_planes_new_sagittal_action, *clip_planes_new_coronal_action;
            QAction *clip_planes_reset_axial_action, *clip_planes_reset_sagittal_action, *clip_planes_reset_coronal_action;
            QAction *clip_planes_invert_action, *clip_planes_remove_action, *clip_planes_clear_action;

            class ClipPlaneModel;
            ClipPlaneModel* clip_planes_model;
            QListView* clip_planes_list_view;

            void set_transparency_from_image ();

        };

      }
    }
  }
}

#endif





