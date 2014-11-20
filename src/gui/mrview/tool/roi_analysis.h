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

#ifndef __gui_mrview_tool_roi_analysis_h__
#define __gui_mrview_tool_roi_analysis_h__

#include "gui/mrview/mode/base.h"
#include "gui/mrview/tool/base.h"
#include "gui/color_button.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class ROI : public Base
        {
            Q_OBJECT

          public:
            ROI (Window& main_window, Dock* parent);

            void draw (const Projection& projection, bool is_3D);
            bool process_batch_command (const std::string& cmd, const std::string& args);

          private slots:
            void new_slot ();
            void open_slot ();
            void save_slot ();
            void close_slot ();
            void draw_slot ();
            void erase_slot ();
            void hide_all_slot ();
            void toggle_shown_slot (const QModelIndex&, const QModelIndex&);
            void selection_changed_slot (const QItemSelection &, const QItemSelection &);
            void update_slot (int unused);
            void colour_changed ();
            void opacity_changed (int unused);

          protected:
             class Model;
             QPushButton *hide_all_button, *draw_button, *erase_button;
             Model* list_model;
             QListView* list_view;
             QColorButton* colour_button;
             QSlider *opacity_slider;

             void update_selection ();
             void updateGL() { 
               window.get_current_mode()->update_overlays = true;
               window.updateGL();
             }
             
             void load (VecPtr<MR::Image::Header>& list); 
        };


      }
    }
  }
}

#endif



