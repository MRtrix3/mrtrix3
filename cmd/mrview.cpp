/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 22/01/09.

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

#include <QApplication>

#include "app.h"
#include "progressbar.h"
#include "gui/init.h"
#include "gui/mrview/window.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "the MRtrix image viewer.";

  ARGUMENTS
  + Argument ("image", "an image to be loaded.")
  .optional()
  .allow_multiple()
  .type_image_in ();

  GUI::init ();
}




void run ()
{
  GUI::MRView::Window window;
  window.show();

  if (argument.size()) {
    VecPtr<MR::Image::Header> list;

    for (size_t n = 0; n < argument.size(); ++n) {
      try {
        list.push_back (new Image::Header (argument[n]));
      }
      catch (Exception& e) {
        e.display();
      }
    }

    if (list.size())
      window.add_images (list);
  }

  if (qApp->exec())
    throw Exception ("error running Qt application");
}




