/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "gui/cursor.h"

namespace MR
{
  namespace GUI
  {

    QCursor Cursor::pan_crosshair;
    QCursor Cursor::forward_backward;
    QCursor Cursor::window;
    QCursor Cursor::crosshair;
    QCursor Cursor::inplane_rotate;
    QCursor Cursor::throughplane_rotate;
    QCursor Cursor::draw;
    QCursor Cursor::erase;

    Cursor::Cursor ()
    {
      pan_crosshair = QCursor (QPixmap (":/cursor_pan.svg"), 8, 8);
      forward_backward = QCursor (QPixmap (":/cursor_pan_through_plane.svg"), 8, 8);
      window = QCursor (QPixmap (":/cursor_brightness_contrast.svg"), 8, 8);
      crosshair = QCursor (QPixmap (":/cursor_crosshairs.svg"), 8, 8);
      inplane_rotate = QCursor (QPixmap (":/cursor_rotate_inplane.svg"), 8, 8);
      throughplane_rotate = QCursor (QPixmap (":/cursor_rotate_throughplane.svg"), 8, 8);
      draw = QCursor (QPixmap (":/cursor_draw.svg"), 8, 8);
      erase = QCursor (QPixmap (":/cursor_erase.svg"), 8, 8);
    }

  }
}

