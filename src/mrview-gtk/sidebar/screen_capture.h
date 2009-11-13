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

#ifndef __mrview_sidebar_screen_capture_h__
#define __mrview_sidebar_screen_capture_h__

#include <gtkmm/spinbutton.h>
#include <gtkmm/entry.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/table.h>
#include <gtkmm/frame.h>

#include "ptr.h"
#include "mrview/window.h"
#include "mrview/sidebar/base.h"

namespace MR {
  namespace Viewer {
    namespace SideBar {

      class ScreenCapture : public Base
      {
        public:
          ScreenCapture ();
          ~ScreenCapture ();

          void draw ();

        protected:
          Gtk::Button       snapshot_button, destination_button, cancel_button;
          Gtk::Frame        destination_prefix_frame, destination_folder_frame;
          Gtk::Label        multislice_label, slice_separation_label, oversample_label, destination_prefix_label, destination_folder_label, destination_number_label;
          Gtk::Table        layout_table;
          Gtk::HBox         destination_number_box;
          Gtk::Adjustment   multislice_adjustment, slice_separation_adjustment, oversample_adjustment, destination_number_adjustment;
          Gtk::SpinButton   multislice, slice_separation, oversample, destination_number;
          Point focus;

          Pane* overlay_pane;
          std::string prefix;
          Point normal, previous_focus;
          Glib::RefPtr<Gdk::Pixbuf> pix;
          unsigned char* framebuffer;
          int   number_remaining, OS, OS_x, OS_y, width, height;

          void   on_snapshot ();
          void   on_browse ();
          void   on_cancel ();

          void   snapshot ();
      };

    }
  }
}

#endif


