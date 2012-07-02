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

#include "gui/mrview/transform.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      void Transform::render_crosshairs (const Point<>& focus) const
      {
        glPushAttrib (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthMask (GL_FALSE);
        Point<> F = model_to_screen (focus);
        F[0] -= x_position();
        F[1] -= y_position();

        glMatrixMode (GL_PROJECTION);
        glPushMatrix ();
        glLoadIdentity ();
        glOrtho (0, width(), 0, height(), -1.0, 1.0);
        glMatrixMode (GL_MODELVIEW);
        glPushMatrix ();
        glLoadIdentity ();

        float alpha = 0.5;

        glColor4f (1.0, 1.0, 0.0, alpha);
        glLineWidth (1.0);
        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBegin (GL_LINES);
        glVertex2f (0.0, F[1]);
        glVertex2f (width(), F[1]);
        glVertex2f (F[0], 0.0);
        glVertex2f (F[0], height());
        glEnd ();

        glDisable (GL_BLEND);
        glPopMatrix ();
        glMatrixMode (GL_PROJECTION);
        glPopMatrix ();
        glMatrixMode (GL_MODELVIEW);
        glPopAttrib();
      }

    }
  }
}

