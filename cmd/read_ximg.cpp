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
#include "file/ximg.h"

using namespace MR; 

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "output XIMG fields in human-readable format.",
  NULL
};


ARGUMENTS = {

  Argument ("file", "the XIMG file to be scanned.")
    .allow_multiple()
    .type_file (),

  Argument ()
};


OPTIONS = {
  Option ()
};


EXECUTE {

  for (size_t n = 0; n < argument.size();  n++) {
    try {
      File::XImg reader (argument[n]);
      std::cout << reader << "\n";
    }
    catch (...) { 
      error ("error reading file \"" + argument[n] + "\"");
    }
  }
}
  
