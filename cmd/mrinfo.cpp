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

#include "image/header.h"
#include "app.h"

using namespace MR; 

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "display header information, or extract specific information from the header",
  NULL
};

ARGUMENTS = {
  Argument ("image", "input image", "the input image.").type_image_in (),
  Argument::End
};



OPTIONS = {

  Option ("transform", "output transform`", 
      "write transform matrix to file")
    .append (Argument ("file", "transform file", 
          "the transform matrix file.").type_file ()),

  Option ("gradient", "output DW scheme", 
      "write DW gradient scheme to file")
    .append (Argument ("file", "DW encoding file", 
          "the DW gradient scheme file.").type_file ()),

  Option::End
};



EXECUTE {
  const Image::Header header = argument[0].get_image (); 

  OptionList opt = get_options ("gradient");
  if (opt.size()) {
    if (!header.DW_scheme.is_set()) 
      error ("no gradient file found for image \"" + header.name() + "\"");
    header.DW_scheme.save (opt[0][0].get_string());
    return;
  }

  opt = get_options ("transform");
  if (opt.size()) {
    header.transform().save (opt[0][0].get_string());
    return;
  }

  std::cout << header.description();
}

