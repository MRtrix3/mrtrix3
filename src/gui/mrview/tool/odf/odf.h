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

#ifndef __gui_mrview_tool_odf_odf_h__
#define __gui_mrview_tool_odf_odf_h__

#include "gui/mrview/tool/base.h"
#include "gui/mrview/adjust_button.h"
#include "gui/mrview/spin_box.h"

namespace MR
{
  namespace GUI
  {
    namespace DWI {
      class Renderer;
      class RenderFrame;
    }

    class LightingDock;


    namespace MRView
    {
      namespace Tool
      {

        class ODF_Item;
        class ODF_Model;
        class ODF_Preview;

        class ODF : public Base
        {
            Q_OBJECT

          public:

            ODF (Dock* parent);
            ~ODF();

            void draw (const Projection& projection, bool is_3D, int axis, int slice) override;

            static void add_commandline_options (MR::App::OptionList& options);
            virtual bool process_commandline_option (const MR::App::ParsedOption& opt) override;

          private slots:
            void onWindowChange ();
            void onPreviewClosed ();
            void image_open_slot ();
            void image_close_slot ();
            void show_preview_slot ();
            void hide_all_slot ();
            void selection_changed_slot (const QItemSelection &, const QItemSelection &);
            void mode_change_slot();
            void lmax_slot (int);
            void dirs_slot();
            void shell_slot();
            void adjust_scale_slot ();
            void colour_by_direction_slot (int unused);
            void hide_negative_values_slot (int unused);
            void use_lighting_slot (int unused);
            void lighting_settings_slot (bool unused);
            void updateGL ();
            void update_preview();

            void close_event() override;

          protected:
             ODF_Preview *preview;

             DWI::Renderer *renderer;

             ODF_Model* image_list_model;
             QListView* image_list_view;
             QPushButton *show_preview_button, *hide_all_button;
             QComboBox *type_selector;
             QLabel *lmax_label, *level_of_detail_label;
             SpinBox *lmax_selector, *level_of_detail_selector;
             QLabel *dirs_label, *shell_label;
             QComboBox *dirs_selector, *shell_selector;
             QCheckBox *use_lighting_box, *hide_negative_values_box, *lock_to_grid_box, *main_grid_box;
             QCheckBox *colour_by_direction_box, *interpolation_box;

             AdjustButton *scale;

             LightingDock *lighting_dock;
             GL::Lighting* lighting;

             int lmax, level_of_detail;
             
             void add_images (std::vector<std::string>& list);

             virtual void showEvent (QShowEvent* event) override;
             virtual void closeEvent (QCloseEvent* event) override;

             ODF_Item* get_image ();
             void get_values (Eigen::VectorXf& SH, MRView::Image& image, const Eigen::Vector3f& pos, const bool interp);
             void setup_ODFtype_UI (const ODF_Item*);

             friend class ODF_Preview;

        };

      }
    }
  }
}

#endif





