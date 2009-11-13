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

#ifndef __mrview_display_area_h__
#define __mrview_display_area_h__

#include <gtkmm/table.h>

#include "mrview/pane.h"

namespace MR {
  namespace Viewer {

    class DisplayArea : public Gtk::Table
    {
      public:
        DisplayArea ();

        void   resize (uint rows, uint columns);
        Pane&  operator () (uint row, uint column) { return (*panes[column+row*NC]); }
        Pane&  current () { return (*panes[0]); }

        bool   on_key_press (GdkEventKey* event);
        void   update ();
        void   update (const SideBar::Base* sidebar);

      protected:
        typedef std::vector<RefPtr<Pane> >::iterator  iterator;
        std::vector<RefPtr<Pane> >   panes;
        uint  NR, NC;
        sigc::connection idle_connection;

        int  do_update ();
    };


    inline void DisplayArea::update ()
    {
      if (!idle_connection.connected())
        idle_connection = Glib::signal_idle().connect (sigc::mem_fun (*this, &DisplayArea::do_update));
    }


    inline void DisplayArea::update (const SideBar::Base* sidebar)
    {
      for (iterator i = panes.begin(); i != panes.end(); i++) {
        Pane& pane (**i);
        if (std::find (pane.sidebar.begin(), pane.sidebar.end(), sidebar) != pane.sidebar.end())
          pane.force_update();
      }
    }

  }
}

#endif



