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

#ifndef __mrview_dialog_properties_h__
#define __mrview_dialog_properties_h__

#include <gtkmm/dialog.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/box.h>
#include <gtkmm/scrolledwindow.h>

#include "mrtrix.h"

namespace MR {
  namespace Image { class Object; }
  namespace Viewer {

    class PropertiesDialog : public Gtk::Dialog
    {
      protected:
        class Columns : public Gtk::TreeModel::ColumnRecord {
          public:
            Columns () { add (key); add (value); }
            Gtk::TreeModelColumn<std::string> key, value;
        };

        Columns                       columns;
        Gtk::VBox                     main_box;
        Glib::RefPtr<Gtk::TreeStore>  model;
        Gtk::TreeView                 tree;
        Gtk::ScrolledWindow           scrolled_window;

      public:
        PropertiesDialog (const MR::Image::Object& image);
    };

  }
}

#endif
