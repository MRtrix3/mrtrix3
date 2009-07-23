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

#include <gtkmm/stock.h>

#include "mrview/dialog/error.h"


namespace MR {
  namespace Viewer {



    std::list<ErrorDialog::ErrorMsg> ErrorDialog::messages;
    sigc::connection  ErrorDialog::idle_connection;


    ErrorDialog::ErrorDialog (const std::string& main_message) : 
      Gtk::Dialog ("Error", true, false),
      more ("details"),
      icon (Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_DIALOG)
    {
      set_border_width (5);
      add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK);
      text.set_label (main_message);

      model = Gtk::ListStore::create (columns);
      details.set_model (model);

      details.append_column ("level", columns.icon);
      details.append_column ("message", columns.text);
      details.set_headers_visible (false);

      for (std::list<ErrorMsg>::iterator msg = messages.begin(); msg != messages.end(); ++msg) {
        Gtk::TreeModel::Row row = *(model->append());
        row[columns.icon] = render_icon (( msg->loglevel == 1 ? Gtk::Stock::DIALOG_ERROR : Gtk::Stock::DIALOG_INFO ), Gtk::ICON_SIZE_MENU);
        row[columns.text] = msg->text;
      }

      hbox.set_border_width (10);
      hbox.set_spacing (10);
      hbox.pack_start (icon, Gtk::PACK_SHRINK);
      hbox.pack_start (text);

      details_window.add (details);
      details_window.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
      details_window.set_shadow_type (Gtk::SHADOW_IN);

      more.add (details_window);

      get_vbox()->pack_start (hbox, Gtk::PACK_SHRINK);
      get_vbox()->pack_start (more);

      show_all_children();
    }

    bool ErrorDialog::display_errors ()
    {
      std::list<ErrorMsg>::iterator msg = messages.end();
      while (msg != messages.begin()) {
        msg--;
        if (msg->loglevel <= 1) {
          ErrorDialog dialog (msg->text);
          dialog.run();
          break;
        }
      }
      messages.clear();
      return (false);
    }

    void ErrorDialog::error (const std::string& msg) 
    {
      cmdline_error (msg);
      messages.push_back (ErrorMsg (1, msg));
      if (idle_connection.empty()) 
        idle_connection = Glib::signal_idle().connect (sigc::ptr_fun (&display_errors));
    } 


    void ErrorDialog::info  (const std::string& msg) 
    {
      cmdline_info (msg);
      messages.push_back (ErrorMsg (2, msg)); 
      if (idle_connection.empty()) 
        idle_connection = Glib::signal_idle().connect (sigc::ptr_fun (&display_errors));
    }


  }
}

