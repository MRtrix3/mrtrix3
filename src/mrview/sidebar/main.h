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

#ifndef __mrview_sidebar_main_h__
#define __mrview_sidebar_main_h__

#include <gtkmm/frame.h>
#include <gtkmm/box.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>

#include "mrtrix.h"

#define NUM_SIDEBAR 4

namespace MR {
  namespace Viewer {
    class Window;

    namespace SideBar {

      class Base;

      class Main : public Gtk::Frame
      {
        public:
          Main ();
          static const char* names[]; 

        protected:
          class SelectorEntry : public Gtk::TreeModel::ColumnRecord {
            public:
              SelectorEntry() { add (ID); add (name); }
              Gtk::TreeModelColumn<int> ID;
              Gtk::TreeModelColumn<std::string> name;
          };

          SelectorEntry    entry;
          Gtk::VBox        box;
          Gtk::ComboBox    selector;
          Glib::RefPtr<Gtk::ListStore> selector_list;
          Base*           list[NUM_SIDEBAR];

          void      init (uint index);
          void      on_selector ();

          friend class MR::Viewer::Window;
      };

    }
  }
}

#endif

