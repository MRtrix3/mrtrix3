/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __cursor_h__
#define __cursor_h__

#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    class Cursor
    { NOMEMALIGN
      public:
        Cursor ();

        static QCursor pan_crosshair;
        static QCursor forward_backward;
        static QCursor window;
        static QCursor crosshair;
        static QCursor inplane_rotate;
        static QCursor throughplane_rotate;
        static QCursor draw;
        static QCursor erase;

    };

  }
}

#endif

