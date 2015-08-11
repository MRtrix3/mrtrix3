/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier and David Raffelt, 13/11/09.

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

#ifndef __gui_mrview_tool_tractography_h__
#define __gui_mrview_tool_tractography_h__

#include "gui/mrview/tool/base.h"
#include "gui/projection.h"
#include "gui/mrview/adjust_button.h"

namespace MR
{
  namespace GUI
  {
    namespace GL {
      class Lighting;
    }

    class LightingDock;

    namespace MRView
    {
      namespace Tool
      {
        class Tractography : public Base
        {
            Q_OBJECT

          public:

            class Model;

            Tractography (Dock* parent);

            virtual ~Tractography ();

            void draw (const Projection& transform, bool is_3D, int axis, int slice) override;
            void draw_colourbars () override;
            size_t visible_number_colourbars () override;
            bool crop_to_slab () const { return (do_crop_to_slab && not_3D); }

            static void add_commandline_options (MR::App::OptionList& options);
            virtual bool process_commandline_option (const MR::App::ParsedOption& opt) override;

            QPushButton* hide_all_button;
            float line_thickness;
            bool do_crop_to_slab;
            bool use_lighting;
            bool not_3D;
            float slab_thickness;
            float line_opacity;
            Model* tractogram_list_model;
            QListView* tractogram_list_view;

            GL::Lighting* lighting;

          private slots:
            void tractogram_open_slot ();
            void tractogram_close_slot ();
            void toggle_shown_slot (const QModelIndex&, const QModelIndex&);
            void hide_all_slot ();
            void on_slab_thickness_slot ();
            void on_crop_to_slab_slot (bool is_checked);
            void on_use_lighting_slot (bool is_checked);
            void on_lighting_settings ();
            void opacity_slot (int opacity);
            void line_thickness_slot (int thickness);
            void right_click_menu_slot (const QPoint& pos);
            void colour_track_by_direction_slot ();
            void colour_track_by_ends_slot ();
            void set_track_colour_slot ();
            void randomise_track_colour_slot ();
            void colour_by_scalar_file_slot ();
            void selection_changed_slot (const QItemSelection &, const QItemSelection &);

          protected:
            AdjustButton* slab_entry;
            QMenu* track_option_menu;
            Dock* scalar_file_options;
            LightingDock *lighting_dock;

        };
      }
    }
  }
}

#endif




