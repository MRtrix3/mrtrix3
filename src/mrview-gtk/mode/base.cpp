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

#include "mrview/mode/base.h"
#include "mrview/mode/normal.h"


namespace MR {
  namespace Viewer {
    namespace Mode {

      RefPtr<Base> create (Pane& parent, uint index)
      {
        switch (index) {
          case 0: return (RefPtr<Base> (new Normal (parent)));
          default: return (RefPtr<Base> ());
        }

        return (RefPtr<Base> ());
      }



      void Base::realize () 
      {
      }

      bool Base::on_key_press (GdkEventKey* event) { return (false); }
      bool Base::on_button_press (GdkEventButton* event) { return (false); }
      bool Base::on_button_release (GdkEventButton* event) { return (false); }
      bool Base::on_motion (GdkEventMotion* event) { return (false); }
      bool Base::on_scroll (GdkEventScroll* event) { return (false); }

    }   
  }
}


