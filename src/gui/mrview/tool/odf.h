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

namespace MR
{
  namespace GUI
  {
    namespace DWI {
      class Renderer;
      class RenderFrame;
    }
    namespace Dialog {
      class Lighting;
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
            ~ODF();

            void draw (const Projection& projection, bool is_3D, int axis, int slice);

            static void add_commandline_options (MR::App::OptionList& options);
            virtual bool process_commandline_option (const MR::App::ParsedOption& opt);

          private slots:
            void onWindowChange ();
            void onPreviewClosed ();
            void image_open_slot ();
            void image_close_slot ();
            void show_preview_slot ();
            void hide_all_slot ();
            void selection_changed_slot (const QItemSelection &, const QItemSelection &);
            void colour_by_direction_slot (int unused);
            void hide_negative_lobes_slot (int unused);
            void lmax_slot (int value);
            void use_lighting_slot (int unused);
            void lighting_settings_slot (bool unused);

            void updateGL ();
            void update_preview();
            void adjust_scale_slot ();

            void hide_event() override;

          protected:
             class Model;
             class Image;

             Dock *preview;

             DWI::Renderer *renderer;

             Model* image_list_model;
             QListView* image_list_view;
             QPushButton *show_preview_button, *hide_all_button;
             QCheckBox *use_lighting_box, *hide_negative_lobes_box, *lock_to_grid_box, *main_grid_box;
             QCheckBox *colour_by_direction_box, *interpolation_box;
             QSpinBox *lmax_selector, *level_of_detail_selector;

             AdjustButton *scale;

             Dialog::Lighting *lighting_dialog;
             GL::Lighting* lighting;

             int lmax, level_of_detail;
             
             void add_images (std::vector<std::string>& list);

             virtual void showEvent (QShowEvent* event);
             virtual void closeEvent (QCloseEvent* event);

             Image* get_image ();
             void get_values (Math::Vector<float>& SH, MRView::Image& image, const Point<>& pos, const bool interp);

             friend class ODF_Preview;

        };

      }
    }
  }
}

#endif





