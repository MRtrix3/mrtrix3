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

#include <gtkmm/spinbutton.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/stock.h>

#include "app.h"
#include "dialog/argument.h"
#include "dialog/file.h"
#include "image/object.h"

namespace MR {
  namespace Dialog {


    Argument::Argument (const MR::Argument& argument) :
      arg (argument),
      description_label (arg.lname)
    {
      set_border_width (GUI_SPACING);
      set_spacing (GUI_SPACING);
      pack_start (description_label, Gtk::PACK_SHRINK);

      switch (arg.type) {
        case Integer: 
          {
            Gtk::SpinButton* sb = manage (new Gtk::SpinButton);
            sb->set_value (arg.extra_info.i.def);
            sb->set_range (arg.extra_info.i.min, arg.extra_info.i.max);
            sb->set_increments (1, 10);
            pack_start (*sb);
            widget = (void*) sb;
          }
          break;
        case Float:
          {
            Gtk::SpinButton* sb = manage (new Gtk::SpinButton);
            sb->set_range (arg.extra_info.f.min, arg.extra_info.f.max);
            sb->set_increments (
                1e-4 *(arg.extra_info.f.max-arg.extra_info.f.min), 
                1e-2 *(arg.extra_info.f.max-arg.extra_info.f.min));
            sb->set_digits (4);
            sb->set_value (arg.extra_info.f.def);
            pack_start (*sb);
            widget = (void*) sb;
          }
          break;
        case Text:
        case IntSeq:
        case FloatSeq: 
          { 
            Gtk::Entry* entry = manage (new Gtk::Entry);
            pack_start (*entry);
            widget = (void*) entry;
          }
          break;
        case ArgFile:
        case ImageIn:
        case ImageOut:
          { 
            Gtk::Entry* entry = manage (new Gtk::Entry);
            Gtk::Button* browse = manage (new Gtk::Button (Gtk::Stock::OPEN));
            browse->signal_clicked().connect (sigc::mem_fun (*this, &Argument::on_browse));
            pack_start (*entry);
            pack_start (*browse, Gtk::PACK_SHRINK);
            widget = (void*) entry;
          }
          break;
        case Choice:
          {
            Gtk::ComboBoxText* choice = manage (new Gtk::ComboBoxText);
            for (const char** p = arg.extra_info.choice; *p; p++) choice->append_text (*p);
            choice->set_active (0);
            pack_start (*choice);
            widget = (void*) choice;
          }
          break;
        default: break;
      }

      set_tooltip_text (arg.desc);
    }






    ArgBase Argument::get () const 
    {
      ArgData* data = new ArgData;
      ArgBase ret;
      ret.data = data;

      switch (arg.type) {
        case Integer: 
          data->data.i = (int) ((Gtk::SpinButton*) widget)->get_value();
          break;
        case Float:
          data->data.f = ((Gtk::SpinButton*) widget)->get_value();
          break;
        case ImageIn:
          if (!image) return (ArgBase());
          data->image = image;
        case Text:
        case ArgFile:
        case ImageOut:
        case IntSeq:
        case FloatSeq: 
          buf = ((Gtk::Entry*) widget)->get_text();
          data->data.string = buf.c_str();
          break;
        case Choice:
          data->data.i = ((Gtk::ComboBoxText*) widget)->get_active_row_number();
          break;
        default: return (ret);
      }

      data->type = arg.type;

      return (ret);
    }






    void Argument::on_browse ()
    {
      bool is_image = ( arg.type == ImageIn || arg.type == ImageOut );
      Dialog::File dialog (arg.lname, false, is_image);

      if (dialog.run() == Gtk::RESPONSE_OK) {
        if (dialog.get_selection().size()) {
          ((Gtk::Entry*) widget)->set_text (dialog.get_selection()[0]);
          if (arg.type == ImageIn) image = dialog.get_images()[0];
        }
      }

    }




  }
}

