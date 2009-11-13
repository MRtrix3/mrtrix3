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

#include <gtkmm/progressbar.h>
#include <gtkmm/main.h>

#include "progressbar.h"
#include "mrview/dialog/progress.h"

namespace MR {
  namespace Viewer {

    ProgressDialog* ProgressDialog::dialog = NULL;

    void ProgressDialog::init () 
    { 
      assert (dialog == NULL);
      dialog = new ProgressDialog (ProgressBar::message);
      dialog->show();
      Gtk::Main::iteration (false);
    }


    void ProgressDialog::display ()
    {
      if (isnan (ProgressBar::multiplier)) dialog->bar.pulse();
      else dialog->bar.set_fraction (ProgressBar::percent/100.0);
      Gtk::Main::iteration (false);
    }


    void ProgressDialog::done () 
    { 
      delete dialog;
      dialog = NULL;
    }

  }
}

