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

#ifndef __gui_mrview_mode_volume_extra_controls_h__
#define __gui_mrview_mode_volume_extra_controls_h__

#include "gui/opengl/lighting.h"
#include "math/vector.h"
#include "gui/mrview/mode/volume.h"
#include "gui/mrview/tool/base.h"
#include "gui/mrview/adjust_button.h"
#include "gui/dialog/lighting.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

          class __ExtraControls : public Tool::Base 
          {
            Q_OBJECT

            public:
              __ExtraControls (Window& main_window, Tool::Dock* parent);
            protected:
              virtual void showEvent (QShowEvent* event);
              virtual void closeEvent (QCloseEvent* event);

            private slots:
              void onScalingChanged ();
              void onSetTransparency ();
              void onUseLighting (bool on);
              void onAdvandedLighting ();
            private:
              AdjustButton *transparent_intensity, *opaque_intensity;
              QComboBox *projection_combobox;
              QGroupBox *transparency_box, *lighting_box;
              QSlider *opacity;
              Dialog::Lighting* lighting_dialog;

              void set_scaling_rate () {
                if (!window.image()) return;
                float rate = window.image()->scaling_rate();
                transparent_intensity->setRate (rate);
                opaque_intensity->setRate (rate);
              }
          };

      }
    }
  }
}

#endif

