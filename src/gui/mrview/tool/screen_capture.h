/*
   Copyright 2013 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt 10/04/2013

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

#ifndef __gui_mrview_tool_screen_capture_h__
#define __gui_mrview_tool_screen_capture_h__

#include <QVBoxLayout>
#include <QSpinBox>
#include "gui/mrview/tool/base.h"
#include "gui/mrview/adjust_button.h"

class QComboBox;

namespace MR
{
  namespace GUI
  {

    namespace MRView
    {
      class AdjustButton;

      namespace Tool
      {


        class ScreenCapture : public Base
        {
          Q_OBJECT
          public:
          ScreenCapture (Window& main_window, Dock* parent);

          private slots:
            void on_screen_capture ();
            bool select_output_folder_slot();

          private:

            AdjustButton *rotaton_axis_x;
            AdjustButton *rotaton_axis_y;
            AdjustButton *rotaton_axis_z;
            AdjustButton *degrees_button;
            QSpinBox *start_index;
            QSpinBox *frames;
            QLineEdit *prefix_textbox;
            QPushButton *folder_button;
            int axis;
            QDir* directory;

        };

      }
    }
  }
}

#endif





