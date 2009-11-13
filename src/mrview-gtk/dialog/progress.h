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

#ifndef __mrview_dialog_progress_h__
#define __mrview_dialog_progress_h__

#include <gtkmm/dialog.h>
#include <gtkmm/label.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/stock.h>

#include "mrtrix.h"

namespace MR {
  namespace Viewer {

    class ProgressDialog : public Gtk::Dialog {
      public:
        static void init ();
        static void display ();
        static void done ();

      protected:
        ProgressDialog (const std::string& message) : Gtk::Dialog ("MRView", false, false), text (message)
      {
        set_border_width (5);
        add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        get_vbox()->pack_start (text, Gtk::PACK_SHRINK);
        get_vbox()->pack_start (bar, Gtk::PACK_SHRINK);
        show_all_children();
      }

        Gtk::Label              text;
        Gtk::ProgressBar        bar;

        static ProgressDialog*         dialog;
    };

  }
}

#endif

