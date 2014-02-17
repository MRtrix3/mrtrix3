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

    Cursor::Cursor ()
    {
      pan_crosshair = QCursor (QPixmap (":/cursor_pan.svg"), 8, 8);
      forward_backward = QCursor (QPixmap (":/cursor_pan_through_plane.svg"), 8, 8);
      window = QCursor (QPixmap (":/cursor_brightness_contrast.svg"), 8, 8);
      crosshair = QCursor (QPixmap (":/cursor_crosshairs.svg"), 8, 8);
      inplane_rotate = QCursor (QPixmap (":/cursor_rotate_inplane.svg"), 8, 8);
      throughplane_rotate = QCursor (QPixmap (":/cursor_rotate_throughplane.svg"), 8, 8);
    }

  }
}

