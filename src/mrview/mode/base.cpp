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

#include "opengl/gl.h"
#include "mrview/mode/base.h"
#include "mrview/mode/list.h"

namespace MR {
  namespace Viewer {
    namespace Mode {

      Base::Base (Window& parent) : 
        window (parent) { }
      Base::~Base () { }

      void Base::paint () { }
      void Base::updateGL () { reinterpret_cast<QGLWidget*> (window.glarea)->updateGL(); }

      void Base::mousePressEvent (QMouseEvent *event) { }
      void Base::mouseMoveEvent (QMouseEvent *event) { }
      void Base::mouseDoubleClickEvent (QMouseEvent* event) { }
      void Base::mouseReleaseEvent (QMouseEvent* event) { }
      void Base::wheelEvent (QWheelEvent* event) { }

      Base* create (Window& parent, size_t index) 
      {
        switch (index) {
#include "mrview/mode/list.h"
          default: assert (0);
        };
        return (NULL);
      }

      const char* name (size_t index)
      {
        switch (index) {
#include "mrview/mode/list.h"
          default: return (NULL);
        };
        return (NULL);
      }

      const char* tooltip (size_t index)
      {
        switch (index) {
#include "mrview/mode/list.h"
          default: return (NULL);
        };
        return (NULL);
      }

    }
  }
}

#undef MODE


