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


    31-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * remove obsolete -dicom & -csa options
   
*/

#include "image/header.h"
#include "app.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "display header information",
  NULL
};

ARGUMENTS = {
  Argument ("image", "input image", "the input image.", AllowMultiple).type_image_in (),
  Argument::End
};



OPTIONS = {
  Option ("grad", "output DW scheme", "write DW gradient scheme to file").append (Argument ("file", "DW encoding file", "the DW gradient scheme file.").type_file ()),
  Option::End
};



EXECUTE {

  std::vector<OptBase> opt = get_options ("grad");
  if (opt.size()) {
    if (argument.size() != 1)
      throw Exception ("Please specify a single image when using the \"-grad\" option");

    const Image::Header header = argument[0].get_image (); 

    if (!header.DW_scheme.is_set()) 
      error ("no gradient file found for image \"" + header.name() + "\"");

    header.DW_scheme.save (opt[0][0].get_string());
  
  }
  else {
    for (size_t i = 0; i < argument.size(); i++) {
      const Image::Header header = argument[i].get_image (); 
      cout << header.description();
    }
  }
}

