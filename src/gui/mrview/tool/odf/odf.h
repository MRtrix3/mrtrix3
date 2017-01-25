/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __gui_mrview_tool_odf_odf_h__
#define __gui_mrview_tool_odf_odf_h__

#include "gui/color_button.h"
#include "gui/mrview/tool/base.h"
#include "gui/mrview/tool/odf/type.h"
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
            void onPreviewClosed ();
            void sh_open_slot ();
            void tensor_open_slot ();
            void dixel_open_slot ();
            void image_close_slot ();
            void show_preview_slot ();
            void hide_all_slot ();
            void selection_changed_slot (const QItemSelection &, const QItemSelection &);
            void lmax_slot (int);
            void dirs_slot();
            void shell_slot();
            void adjust_scale_slot ();
            void colour_by_direction_slot (int unused);
            void hide_negative_values_slot (int unused);
            void colour_change_slot();
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
             QLabel *lmax_label, *level_of_detail_label;
             SpinBox *lmax_selector, *level_of_detail_selector;
             QLabel *dirs_label, *shell_label;
             QComboBox *dirs_selector, *shell_selector;
             QCheckBox *use_lighting_box, *hide_negative_values_box, *lock_to_grid_box, *main_grid_box;
             QCheckBox *colour_by_direction_box, *interpolation_box;
             QColorButton *colour_button;

             AdjustButton *scale;

             LightingDock *lighting_dock;
             GL::Lighting* lighting;

             int lmax;
             
             void add_images (std::vector<std::string>& list, const odf_type_t mode);

             virtual void closeEvent (QCloseEvent* event) override;

             ODF_Item* get_image ();
             void get_values (Eigen::VectorXf&, ODF_Item&, const Eigen::Vector3f&, const bool);
             void setup_ODFtype_UI (const ODF_Item*);

             friend class ODF_Preview;

        };

      }
    }
  }
}

#endif





