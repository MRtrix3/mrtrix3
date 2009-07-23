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

#ifndef __dialog_argument_h__
#define __dialog_argument_h__

#include <gtkmm/box.h>
#include <gtkmm/label.h>

#include "args.h"
#include "refptr.h"

namespace MR {
  namespace Image { class Object; }

  namespace Dialog {

    class Argument : public Gtk::HBox {
      public:
        const MR::Argument&  arg;
        Gtk::Label           description_label;
        RefPtr<Image::Object> image;
        void*                widget;
        mutable std::string       buf;

        void                 on_browse ();

        Argument (const MR::Argument& argument);

        ArgBase              get () const;
    };

  }
}

#endif

