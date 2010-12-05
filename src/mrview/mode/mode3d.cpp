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

#include "mrview/mode/mode3d.h"

namespace MR {
  namespace Viewer {
    namespace Mode {

      Mode3D::Mode3D (Window& parent) : Base (parent) { }
      Mode3D::~Mode3D () { }

      void Mode3D::paint () 
      { 
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (!image()) {
          renderText (10, 10, "No image loaded");
          return;
        }
        if (!target()) set_target (focus());

        // camera target:
        //Point<> F = target();

        // info for projection:
        int w = glarea()->width(), h = glarea()->height();
        float fov = FOV() / (float) (w+h);
        float depth = 100.0;

        // set up projection & modelview matrices:
        glMatrixMode (GL_PROJECTION);
        glLoadIdentity ();
        glOrtho (-w*fov, w*fov, -h*fov, h*fov, -depth, depth);

        glMatrixMode (GL_MODELVIEW);
        glLoadIdentity ();
        //glMultMatrixf (M);
        //glTranslatef (-F[0], -F[1], -F[2]);
        get_modelview_projection_viewport();

        // set up OpenGL environment:
        glDisable (GL_BLEND);
        glEnable (GL_TEXTURE_3D);
        glShadeModel (GL_FLAT);
        glDisable (GL_DEPTH_TEST);
        glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glDepthMask (GL_FALSE);
        glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        Math::Quaternion view;
        // render image:
        DEBUG_OPENGL;
        image()->render3D (*this);
        DEBUG_OPENGL;

        glDisable (GL_TEXTURE_3D);
      }

      bool Mode3D::mouse_click () { return (false); }
      bool Mode3D::mouse_move () { return (false); }
      bool Mode3D::mouse_doubleclick () { return (false); }
      bool Mode3D::mouse_release () { return (false); }
      bool Mode3D::mouse_wheel (float delta, Qt::Orientation orientation) { return (false); }

    }
  }
}


