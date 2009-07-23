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

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "output XIMG fields in human-readable format.",
  NULL
};


ARGUMENTS = {
  Argument ("file", "XIMG file", "the XIMG file to be scanned.", AllowMultiple).type_file (),
  Argument::End
};


OPTIONS = {
  Option::End
};


EXECUTE {
  File::XImg reader;

  for (size_t n = 0; n < argument.size();  n++) {
/*
    if (Glib::file_test (argument[n].get_string(), Glib::FILE_TEST_IS_DIR)) {
      Glib::Dir* dir;
      try { dir = new Glib::Dir (argument[n].get_string()); }
      catch (...) { throw Exception (std::string ("error opening folder \"") + argument[n].get_string() 
          + "\": " + Glib::strerror (errno)); }
      
      std::string entry;
      while ((entry = dir->read_name()).size()) {
        if (reader.read (Glib::build_filename (argument[n].get_string(), entry), print_DICOM_fields, print_CSA_fields))
          error ("error reading file \"" + reader.filename + "\"");
        else cout << reader << "\n";
      }
    }
    else if (reader.read (argument[n].get_string(), print_DICOM_fields, print_CSA_fields))
      error ("error reading file \"" + reader.filename + "\"");
*/
    try {
      reader.read (argument[n].get_string());
      cout << reader << "\n";
    }
    catch (...) { 
      error (std::string ("error reading file \"") + argument[n].get_string() + "\"");
    }

  }

}
  
