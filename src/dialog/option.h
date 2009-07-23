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

#ifndef __dialog_option_decl_h__
#define __dialog_option_decl_h__

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/button.h>
#include <gtkmm/separator.h>

#include "args.h"

namespace MR {
  namespace Dialog {

    class Option : public Gtk::VBox {
      public:
        const MR::Option&    opt;
        Window&              window;
        Gtk::HSeparator      line;
        Gtk::HBox            top_box;
        Gtk::Label           description_label;
        Gtk::Button          remove_button;

        void                 on_remove_button ();

        Option (Window& parent, const MR::Option& option, uint index);
        uint              idx;
        OptBase            get () const;
    };

  }
}

#endif

