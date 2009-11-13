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

#include "mrview/display_area.h"

namespace MR {
  namespace Viewer {


    DisplayArea::DisplayArea () 
    {
      set_homogeneous (true);
      resize (1,1);
      show_all();
    }




    void DisplayArea::resize (uint rows, uint columns)
    {
      NR = rows;
      NC = columns;
      Gtk::Table::resize (NR, NC);
      panes.resize (NR*NC);

      for (iterator it = panes.begin(); it != panes.end(); ++it) if (!(*it)) *it = new Pane;

      for (uint row = 0; row < NR; row++)
        for (uint col = 0; col < NC; col++)
          attach (*panes[col+row*NC], col, col+1, row, row+1);
    }






    bool DisplayArea::on_key_press (GdkEventKey* event)
    {
      if (event->is_modifier) return (false);
      return (current().on_key_press (event));
    }






    int DisplayArea::do_update () 
    { 
      for (iterator i = panes.begin(); i != panes.end(); i++)
        (*i)->do_update();
      return (0);
    }


  }
}

