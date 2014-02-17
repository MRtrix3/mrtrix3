#ifndef __cursor_h__
#define __cursor_h__

#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    class Cursor
    {
      public:
        Cursor ();

        static QCursor pan_crosshair;
        static QCursor forward_backward;
        static QCursor window;
        static QCursor crosshair;
        static QCursor inplane_rotate;
        static QCursor throughplane_rotate;

    };

  }
}

#endif

