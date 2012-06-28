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

#ifndef __gui_mrview_mode_mode3d_h__
#define __gui_mrview_mode_mode3d_h__

#include "app.h"
#include "gui/mrview/mode/mode2d.h"

#define ROTATION_INC 0.002

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        class Mode3D : public Mode2D
        {
          public:
            Mode3D (Window& parent, int flags = FocusContrast | MoveTarget | TiltRotate);
            virtual ~Mode3D ();

            virtual void paint ();

            virtual void reset_event ();
            virtual void slice_move_event (int x);
            virtual void set_focus_event ();
            virtual void contrast_event ();
            virtual void pan_event ();
            virtual void panthrough_event ();
            virtual void tilt_event ();
            virtual void rotate_event ();

            static const App::OptionGroup options;

          protected:
            void draw_orientation_labels ();
        };

      }
    }
  }
}

#endif




