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

#ifndef __mrview_dialog_error_h__
#define __mrview_dialog_error_h__

#include <gtkmm/dialog.h>
#include <gtkmm/expander.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/label.h>
#include <gtkmm/image.h>
#include <gtkmm/box.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>


#include "app.h"

namespace MR {
  namespace Viewer {

    class ErrorDialog : public Gtk::Dialog {
      public:
        ErrorDialog (const std::string& main_message);

        static void error (const std::string& msg);
        static void info  (const std::string& msg);

      protected:
        Gtk::HBox               hbox;
        Gtk::ScrolledWindow     details_window;
        Gtk::Expander           more;
        Gtk::Label              text;
        Gtk::Image              icon;

        class Columns : public Gtk::TreeModel::ColumnRecord {
          public:
            Columns() { add (icon); add (text); }
            Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
            Gtk::TreeModelColumn<std::string> text;
        };

        Columns columns;
        Glib::RefPtr<Gtk::ListStore> model;
        Gtk::TreeView details;

        class ErrorMsg {
          public:
            ErrorMsg (int log_level, const std::string& message) : loglevel (log_level), text (message) { }
            int    loglevel;
            std::string text;
        };
        static std::list<ErrorMsg> messages;

        static sigc::connection idle_connection;
        static bool display_errors ();
    };

  }
}

#endif

