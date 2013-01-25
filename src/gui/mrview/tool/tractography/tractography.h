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

class QStringListModel;

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class Tractography : public Base
        {
            Q_OBJECT

          public:

            Tractography (Window& main_window, Dock* parent);

            ~Tractography ();

            void draw2D (const Projection& transform);
            void draw3D (const Projection& transform);

            float get_line_thickness () const { return line_thickness; }
            bool do_crop_to_slab () const { return crop_to_slab; }
            float get_slab_thickness () const { return slab_thickness; }
            bool do_shader_update () const { return shader_update; }
            float get_opacity () const { return line_opacity; }

          private slots:
            void tractogram_open_slot ();
            void tractogram_close_slot ();
            void toggle_shown (const QModelIndex& index);
            void on_slab_thickness_change();
            void on_crop_to_slab_change (bool checked);
            void opacity_slot (int opacity);
            void line_thickness_slot (int thickness);
            void show_right_click_menu (const QPoint& pos);

          protected:
             class Model;
             Model* tractogram_list_model;
             QListView* tractogram_list_view;
             AdjustButton* slab_entry;
             float line_thickness;
             bool crop_to_slab;
             float slab_thickness;
             bool shader_update;
             float line_opacity;
        };

      }
    }
  }
}

#endif




