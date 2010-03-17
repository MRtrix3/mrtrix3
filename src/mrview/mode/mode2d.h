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

#ifndef __viewer_mode_mode2d_h__
#define __viewer_mode_mode2d_h__

#include "mrview/mode/base.h"

namespace MR {
  namespace Viewer {
    namespace Mode {

      class Mode2D : public Base 
      {
        Q_OBJECT

        public:
          Mode2D (Window& parent);
          virtual ~Mode2D ();

          virtual void paint ();

          virtual void mousePressEvent (QMouseEvent* event);
          virtual void mouseMoveEvent (QMouseEvent* event);
          virtual void mouseDoubleClickEvent (QMouseEvent* event);
          virtual void mouseReleaseEvent (QMouseEvent* event);
          virtual void wheelEvent (QWheelEvent* event);

        public slots:
          void axial ();
          void sagittal ();
          void coronal ();
          void reset ();
          void show_focus ();

        protected:
          QAction *axial_action, *sagittal_action, *coronal_action, *reset_action, *show_focus_action;
          Point lookat;
          Qt::MouseButtons buttons;
          Qt::KeyboardModifiers modifiers;
          QPoint pos;
      };

    }
  }
}

#endif



