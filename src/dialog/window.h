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

#ifndef __dialog_window_h__
#define __dialog_window_h__

#include <gtkmm/button.h>
#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/textview.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/paned.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/progressbar.h>


#include "app.h"

namespace MR {
  namespace Dialog {

    class Argument;
    class Option;

    class Window : public Gtk::Window {
      protected:
        App&                    app;
        Gtk::VBox               arguments_box, top_box, bottom_box, inner_description_box, options_box;
        Gtk::HBox               message_level_box, button_box, option_menu_box;
        Gtk::Button             close_button, stop_button, start_button;
        Gtk::Label              description_label, message_level_label, options_label;
        Gtk::ScrolledWindow     top;
        Gtk::VPaned             splitter;
        Gtk::Frame              description_frame, arguments_frame, options_frame; 
        Gtk::Frame              inner_text_frame, text_frame;
        Gtk::ComboBoxText       message_level, option_combobox;

        static const char* message_level_options[3];

        void                    on_close_button ();
        void                    on_stop_button ();
        void                    on_start_button ();
        void                    on_add_option ();
        
        void                    set_option_list ();


      public:
        Window (App& parent);

        Gtk::ProgressBar        progressbar;
        Gtk::TextView           text;
        Glib::RefPtr<Gtk::TextBuffer::Tag> red, blue, grey;

        void                    remove_option (Option& option);
    };

  }
}

#endif

