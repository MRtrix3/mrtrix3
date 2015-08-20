/*
   Copyright 2015 Brain Research Institute, Melbourne, Australia

   Written by Rami Tabbara, 18/02/15.

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

#ifndef __gui_mrview_mode_lightbox_gui_h__
#define __gui_mrview_mode_lightbox_gui_h__

#include "gui/mrview/spin_box.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {
        namespace LightBoxViewControls
        {

          class LightBoxEditButton : public SpinBox
          {
            public:
              LightBoxEditButton(QWidget* parent, const QString &suffix)
                : LightBoxEditButton(parent, 1, 100, 1, suffix) {}

              LightBoxEditButton(QWidget* parent,
                  int min = 1, int max = 100, int change_rate = 1,
                  const QString& suffix = nullptr) :
                SpinBox(parent)
            {
              setMinimum(min);
              setMaximum(max);
              setSingleStep(change_rate);
              setSuffix(suffix);

              setMaximumWidth(80);
            }
          };

        }

      }
    }
  }
}


#endif
