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

      void Mode3D::paint () { 
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      }

      bool Mode3D::mouse_click () { return (false); }
      bool Mode3D::mouse_move () { return (false); }
      bool Mode3D::mouse_doubleclick () { return (false); }
      bool Mode3D::mouse_release () { return (false); }
      bool Mode3D::mouse_wheel (float delta, Qt::Orientation orientation) { return (false); }

    }
  }
}


