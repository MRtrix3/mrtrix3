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

#include "command.h"
#include "image/header.h"


using namespace MR;
using namespace App;

void usage () {

  DESCRIPTION =
    Description()
    + "display header information, or extract specific information from the header";

  ARGUMENTS =
    ArgumentList() 
    + Argument ("image", 
        "the input image.").allow_multiple().type_image_in ();

  OPTIONS = 
    OptionList() + OptionGroup()

    + Option ("transform", "write transform matrix to file")
    +   Argument ("file").type_file ();

}


void run () {

  Options opt = get_options ("transform");
  if (opt.size()) {
    if (argument.size() > 1)
      throw Exception ("only a single input image is allowed when writing image header transform to file");
    Image::Header header (argument[0]);
    header.transform().save (opt[0][0]);
    return;
  }

  for (size_t i = 0; i < argument.size(); ++i) {
    Image::Header header (argument[i]);
    std::cout << header.description();
  }
}

