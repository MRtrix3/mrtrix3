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

#include "app.h"
#include "image/header.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage () {

  DESCRIPTION =
    Description()
    + "display header information, or extract specific information from the header";

  ARGUMENTS =
    ArgumentList() 
    + Argument ("image", 
        "the input image.").type_image_in ();

  OPTIONS = 
    OptionList() + OptionGroup()

    + Option ("transform", "write transform matrix to file")
    +   Argument ("file").type_file ()

    + Option ("gradient", "write DW gradient scheme to file")
    +   Argument ("file").type_file ();

}


void run () {
  Image::Header header (argument[0]);

  Options opt = get_options ("gradient");
  if (opt.size()) {
    if (!header.DW_scheme().is_set())
      error ("no gradient file found for image \"" + header.name() + "\"");
    header.DW_scheme().save (opt[0][0]);
    return;
  }

  opt = get_options ("transform");
  if (opt.size()) {
    header.transform().save (opt[0][0]);
    return;
  }

  std::cout << header.description();
}

