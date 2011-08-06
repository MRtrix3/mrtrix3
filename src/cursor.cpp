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

#include <QImage>
#include <QPixmap>

#include "cursor.h"
#include "icons.h"

#define CREATE_CURSOR(name,offx,offy) \
{ \
  QImage image (Icon::name.data, Icon::name.width, Icon::name.height, QImage::Format_ARGB32); \
  name = QCursor (QPixmap::fromImage (image), offx, offy); \
}

namespace MR {

  QCursor Cursor::pan_crosshair;
  QCursor Cursor::forward_backward;
  QCursor Cursor::pan;
  QCursor Cursor::window;
  QCursor Cursor::crosshair;
  QCursor Cursor::zoom;
  QCursor Cursor::inplane_rotate;
  QCursor Cursor::throughplane_rotate;

  Cursor::Cursor ()
  {
    CREATE_CURSOR (pan_crosshair, 9, 8);
    CREATE_CURSOR (forward_backward, 9, 8);
    CREATE_CURSOR (pan, 16, 16);
    CREATE_CURSOR (window, 9, 8);
    CREATE_CURSOR (crosshair, 9, 8);
    CREATE_CURSOR (zoom, 9, 8);
    CREATE_CURSOR (inplane_rotate, 9, 8);
    CREATE_CURSOR (throughplane_rotate, 9, 8);
  }

}

