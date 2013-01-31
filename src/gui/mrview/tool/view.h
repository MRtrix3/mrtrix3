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

#include <QVBoxLayout>

#include "gui/mrview/tool/base.h"

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


        class View : public Base
        {
          Q_OBJECT
          public:
            View (Window& main_window, Dock* parent);

          protected:
            virtual void showEvent (QShowEvent* event);
            virtual void closeEvent (QCloseEvent* event);

          private slots:
            void onImageChanged ();
            void onFocusChanged ();
            void onSetFocus ();
            void onPlaneChanged ();
            void onSetPlane (int index);
            void onSetScaling ();
            void onScalingChanged ();
            void onSetColourBar (int index);

          private:
            AdjustButton *focus_x, *focus_y, *focus_z; 
            AdjustButton *max_entry, *min_entry;
            // AdjustButton *lessthan, *greaterthan;
            QComboBox *plane_combobox, *colourbar_combobox;

            void set_scaling_rate ();
            void set_focus_rate ();

        };

      }
    }
  }
}

#endif





