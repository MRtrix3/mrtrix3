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

class QLineEdit;
class QComboBox;
class QSlider;
class QGroupBox;

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        class View : public Base
        {
          Q_OBJECT
          public:
            View (Dock* parent);

          protected:
            virtual void showEvent (QShowEvent* event);
            virtual void closeEvent (QCloseEvent* event);

          private slots:
            void onFocusChanged ();
            void onSetFocus ();
            void onProjectionChanged ();
            void onSetProjection (int index);
            void onSetScaling ();
            void onScalingChanged ();
            void onSetThreshold ();
            void onSetTransparency ();

          private:
            QLineEdit *focus_x, *focus_y, *focus_z, *min_entry, *max_entry, *lessthan, *greaterthan;
            QLineEdit *transparent_intensity, *opaque_intensity;
            QComboBox *projection_combobox;
            QGroupBox *threshold_box, *transparency_box;
            QSlider *opacity;

        };

      }
    }
  }
}

#endif





