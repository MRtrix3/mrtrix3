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

#include "dialog/window.h"
#include "dialog/option.h"
#include "dialog/argument.h"

namespace MR {
  namespace Dialog {


    Option::Option (Window& parent, const MR::Option& option, uint index) :
      opt (option),
      window (parent),
      description_label (option.lname),
      remove_button ("remove"),
      idx (index)
    {
      if (opt.mandatory && !opt.allow_multiple) remove_button.set_sensitive (false);
      top_box.pack_start (description_label, Gtk::PACK_SHRINK);
      top_box.pack_end (remove_button, Gtk::PACK_SHRINK);
      pack_start (line, Gtk::PACK_SHRINK);
      pack_start (top_box, Gtk::PACK_SHRINK);
      set_spacing (GUI_SPACING);

      remove_button.signal_clicked().connect (sigc::mem_fun(*this, &Option::on_remove_button));

      for (uint n = 0; n < opt.size(); n++) {
        Argument* argument = manage (new Argument (opt[n]));
        pack_start (*argument, Gtk::PACK_SHRINK);
      }

      set_tooltip_text (opt.desc);
    }






    OptBase Option::get () const 
    {
      OptBase ret;
      ret.index = UINT_MAX;
/*
      wxSizerItemList list = main_sizer->GetChildren();
      for (uint n = 2; n < list.GetCount(); n++) {
        Argument* arg = (Argument*) (list.Item(n)->GetData()->GetWindow());
        ret.append (arg->get());
        if (ret.last().type() == Undefined) {
          wxLogError (wxT("value supplied for argument \"%s\" of option \"%s\" is not valid"), arg->arg.lname, opt.lname);
          return (ret);
        }
      }
  */    

      ret.index = idx;
      return (ret);
    }






    void Option::on_remove_button () { window.remove_option (*this); }



  }
}

