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

#ifndef __viewer_mode_base_h__
#define __viewer_mode_base_h__

#include <QCursor>
#include <QMouseEvent>
#include <QMenu>

#include "mrview/window.h"

namespace MR {
  namespace Viewer {
    namespace Mode {

      class Base : public QObject
      {
        Q_OBJECT

        public:
          Base (Window& parent);
          virtual ~Base ();

          virtual void paint ();

          virtual void mousePressEvent (QMouseEvent* event);
          virtual void mouseMoveEvent (QMouseEvent* event);
          virtual void mouseDoubleClickEvent (QMouseEvent* event);
          virtual void mouseReleaseEvent (QMouseEvent* event);
          virtual void wheelEvent (QWheelEvent* event);


        protected:
          Window& window;
          QPoint lastPos;

          QPoint distance_moved (QMouseEvent* event) { 
            QPoint d = event->pos() - lastPos; 
            lastPos = event->pos();
            return (d);
          }

          QPoint distance_moved_motionless (QMouseEvent* event) {
            QPoint d = event->pos() - lastPos; 
            QCursor::setPos (window.global_position (lastPos));
            return (d);
          }

          void add_action (QAction* action)
          {
            window.view_menu->insertAction (window.view_menu_mode_area, action);
          }
      };

      Base* create (Window& parent, size_t index);
      const char* name (size_t index);

    }
  }
}

#endif


