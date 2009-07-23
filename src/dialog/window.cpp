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
#include <gtkmm/main.h>
#include <gtkmm/widget.h>
#include <gtkmm/messagedialog.h>

#include "dialog/window.h"
#include "dialog/argument.h"
#include "dialog/option.h"

#undef EXECUTE

namespace MR {
  namespace Dialog {

    namespace {

      Window* window = NULL;

      void gui_print (const std::string& msg) { window->text.get_buffer()->insert_at_cursor (msg); }
      
      void gui_error (const std::string& msg) 
      {
        if (App::log_level) {
          window->text.get_buffer()->insert_with_tag (window->text.get_buffer()->end(), msg + "\n", window->red); 
          Gtk::MessageDialog dialog (*window, msg, false, Gtk::MESSAGE_ERROR);
          dialog.run();
        }
      }

      void gui_info  (const std::string& msg) 
      { 
        if (App::log_level > 1) window->text.get_buffer()->insert_with_tag (window->text.get_buffer()->end(), msg + "\n", window->blue); 
      }

      void gui_debug (const std::string& msg)
      { 
        if (App::log_level > 2) window->text.get_buffer()->insert_with_tag (window->text.get_buffer()->end(), msg + "\n", window->grey); 
      }
      
    }
  }


  namespace ProgressBar {
    namespace {

      void init_func_gui ()
      {
        Dialog::window->progressbar.set_fraction (0.0); 
        Dialog::window->text.get_buffer()->insert_at_cursor (message + "... ");
      }


      void display_func_gui ()
      {
        if (isnan (multiplier)) Dialog::window->progressbar.pulse ();
        else Dialog::window->progressbar.set_fraction (percent/100.0);
      }


      void done_func_gui ()
      {
        Dialog::window->progressbar.set_fraction (1.0);
        Dialog::window->text.get_buffer()->insert_at_cursor ("ok\n");
      }


    }
  }




  namespace Dialog {

    const char* Window::message_level_options[] = {
      "error messages only", 
      "error & information messages", 
      "error, information & debugging messages" 
    };



    Window::Window (App& parent) : 
      Gtk::Window (Gtk::WINDOW_TOPLEVEL), 
      app (parent),
      close_button (Gtk::Stock::CLOSE),
      stop_button (Gtk::Stock::STOP),
      start_button (Gtk::Stock::EXECUTE),
      message_level_label ("Message level"),
      options_label ("Add option"),
      description_frame ("Description"),
      arguments_frame ("Arguments"),
      options_frame ("Options"),
      text_frame ("Program output")
    {
      window = this;
      realize();

      description_label.set_line_wrap (true);
      description_label.set_single_line_mode (false);
      description_label.set_justify (Gtk::JUSTIFY_FILL);
      description_label.set_selectable (true);

      if (app.command_description[0]) {
        description_label.set_text (app.command_description[0]);
        if (app.command_description[1]) {
          std::string desc (app.command_description[1]);
          for (const char** p = app.command_description+2; *p; p++) { desc += "\n\n"; desc += *p; }
          description_label.set_tooltip_text (desc);
        }
      }
      else description_label.set_text ("no description available");

      inner_description_box.set_border_width (GUI_SPACING);
      inner_description_box.add (description_label);
      description_frame.add (inner_description_box);

 
      for (const MR::Argument* arg = app.command_arguments; arg->is_valid(); arg++) {
        Argument* argument = manage (new Argument (*arg));
        arguments_box.pack_start (*argument, Gtk::PACK_SHRINK);
      }
      arguments_frame.add (arguments_box);

      message_level.append_text (message_level_options[0]);
      message_level.append_text (message_level_options[1]);
      message_level.append_text (message_level_options[2]);
      message_level.set_active (0);

      message_level_box.set_spacing (GUI_SPACING);
      message_level_box.pack_start (message_level_label, Gtk::PACK_SHRINK);
      message_level_box.pack_start (message_level);

      top_box.set_border_width (GUI_SPACING);
      top_box.set_spacing (GUI_SPACING);
      top_box.pack_start (description_frame);
      top_box.pack_start (arguments_frame);
      
      if (app.command_options[0].is_valid()) {
        options_box.pack_start (option_menu_box, Gtk::PACK_SHRINK);
        options_box.set_spacing (GUI_SPACING);
        options_box.set_border_width (GUI_SPACING);
        option_menu_box.pack_start (options_label, Gtk::PACK_SHRINK);
        option_menu_box.pack_start (option_combobox);
        option_menu_box.set_spacing (GUI_SPACING);
        for (uint n = 0; App::command_options[n].is_valid(); n++) {
          if (App::command_options[n].mandatory) {
            Option* opt = manage (new Option (*this, App::command_options[n], n));
            options_box.pack_start (*opt, Gtk::PACK_SHRINK);
          }
        }
        set_option_list();
        options_frame.add (options_box);
        top_box.pack_start (options_frame);
        option_combobox.signal_changed().connect (sigc::mem_fun (*this, &Window::on_add_option));
      }

      top_box.pack_start (message_level_box, Gtk::PACK_SHRINK);

      top.set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
      top.set_shadow_type (Gtk::SHADOW_NONE);
      top.add (top_box);

      Glib::RefPtr<Gtk::TextBuffer::TagTable> tags = text.get_buffer()->get_tag_table();
      red  = Gtk::TextBuffer::Tag::create(); red->property_foreground()  = "red";  tags->add (red);
      blue = Gtk::TextBuffer::Tag::create(); blue->property_foreground() = "blue"; tags->add (blue);
      grey = Gtk::TextBuffer::Tag::create(); grey->property_foreground() = "grey"; tags->add (grey);

      text.set_editable (false);
      text.set_cursor_visible (false);
      inner_text_frame.set_shadow_type (Gtk::SHADOW_IN);
      inner_text_frame.set_border_width (GUI_SPACING);
      inner_text_frame.add (text);
      text_frame.add (inner_text_frame);

      close_button.signal_clicked().connect (sigc::mem_fun(*this, &Window::on_close_button));
      stop_button.set_sensitive (false);
      stop_button.signal_clicked().connect (sigc::mem_fun(*this, &Window::on_stop_button));
      start_button.signal_clicked().connect (sigc::mem_fun(*this, &Window::on_start_button));

      button_box.set_spacing (GUI_SPACING);
      button_box.set_homogeneous();
      button_box.pack_start (close_button);
      button_box.pack_start (stop_button);
      button_box.pack_start (start_button);



      bottom_box.set_border_width (GUI_SPACING);
      bottom_box.set_spacing (GUI_SPACING);
      bottom_box.pack_start (progressbar, Gtk::PACK_SHRINK);
      bottom_box.pack_start (text_frame);
      bottom_box.pack_start (button_box, Gtk::PACK_SHRINK);


      splitter.pack1 (top, true, false);
      splitter.pack2 (bottom_box, true, false);
      add (splitter);
      set_icon (App::gui_icon);


      ProgressBar::init_func = ProgressBar::init_func_gui;
      ProgressBar::display_func = ProgressBar::display_func_gui;
      ProgressBar::done_func = ProgressBar::done_func_gui;

      print = gui_print;
      error = gui_error;
      info = gui_info;
      debug = gui_debug;



      set_default_size (400, 800);
      show_all_children();
    }


    void Window::on_close_button () { Gtk::Main::quit (); }




    void Window::on_stop_button ()
    {
      ProgressBar::stop = true;
      print ("\nAborted\n\n");
    }



    void Window::on_start_button ()
    {
      app.argument.clear();
      app.option.clear();

      // argument list:
      for (Gtk::Box_Helpers::BoxList::iterator it = arguments_box.children().begin(); it != arguments_box.children().end(); ++it) {
        Argument* arg = (Argument*) (it->get_widget());
        app.argument.push_back (arg->get());
        if (app.argument.back().type() == Undefined) {
          error (std::string ("value supplied for argument \"") + arg->arg.lname + "\" is not valid");
          return;
        }
      }

      ProgressBar::display = true;
      App::log_level = 1 + message_level.get_active_row_number();

      stop_button.set_sensitive (true);
      start_button.set_sensitive (false);

      try { 
        app.execute(); 
        print ("\nCompleted successfully\n\n");
      } 
      catch (...) { error ("Error during execution!"); }

      stop_button.set_sensitive (false);
      start_button.set_sensitive (true);
    }



    void Window::set_option_list ()
    {
      option_combobox.clear_items();
      Gtk::Box_Helpers::BoxList& list (options_box.children());

      option_combobox.append_text ("--");
      for (const MR::Option* opt = App::command_options; opt->is_valid(); opt++) {
        const char* name = opt->lname;
        for (uint n = 1; n < list.size(); n++) {
          Option* p = (Option*) (list[n].get_widget());
          if (opt == &p->opt) {
            if (!opt->allow_multiple) name = NULL;
            break;
          }
        }
        if (name) option_combobox.append_text (name);
      }

      option_combobox.set_active (0);
    }





    void Window::on_add_option ()
    {
      if (option_combobox.get_active_row_number() == 0) return;

      std::string selection (option_combobox.get_active_text ());
      for (uint n = 0; App::command_options[n].is_valid(); n++) {
        if (selection == App::command_options[n].lname) {
          Option* opt = manage (new Option (*this, App::command_options[n], n));
          options_box.pack_start (*opt, Gtk::PACK_SHRINK);
          options_box.show_all();
          set_option_list();
          return;
        }
      }

    }



    void Window::remove_option (Option& option)
    {
      options_box.remove (option);
      delete &option;
      set_option_list();
    }


  }
}

