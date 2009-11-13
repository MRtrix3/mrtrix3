/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __mrview_mode_normal_h__
#define __mrview_mode_normal_h__

#include "mrview/mode/base.h"

namespace MR {
  namespace Viewer {
    namespace Slice { class Current; }

    namespace Mode {

      class Normal : public Base
      {
        public:
          Normal (Pane& parent) : Base (parent, 0) { }
          virtual ~Normal ();

          void      draw ();
          void      configure ();
          void      reset ();
          bool      on_button_press (GdkEventButton* event);
          bool      on_button_release (GdkEventButton* event);
          bool      on_motion (GdkEventMotion* event);
          bool      on_scroll (GdkEventScroll* event);
          bool      on_key_press (GdkEventKey* event);

        protected:
          double    xprev, yprev;

          void      move_slice (Slice::Current& S, float dist);
          void      set_focus (Slice::Current& S, double x, double y);
      };

    }
  }
}

#endif



