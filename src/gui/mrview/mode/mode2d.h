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

#ifndef __gui_mrview_mode_mode2d_h__
#define __gui_mrview_mode_mode2d_h__

#include "gui/mrview/mode/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        class Mode2D : public Base
        {
            Q_OBJECT

          public:
            Mode2D (Window& parent);
            virtual ~Mode2D ();

            virtual void paint ();

            virtual bool mouse_click ();
            virtual bool mouse_move ();
            virtual bool mouse_release ();
            virtual bool mouse_wheel (float delta, Qt::Orientation orientation);

            virtual void reset_view ();

          public slots:
            virtual void axial ();
            virtual void sagittal ();
            virtual void coronal ();
            virtual void slice_prev ();
            virtual void slice_next ();
            virtual void reset ();

          protected:
            QAction *axial_action, *sagittal_action, *coronal_action, *volume_next_action, *volume_previous_action;
            QAction *slice_prev_action, *slice_next_action;

            void set_cursor ();
        };

      }
    }
  }
}

#endif



