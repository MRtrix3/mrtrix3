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
#include "file/path.h"
#include "file/dicom/quick_scan.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "output DICOM fields in human-readable format.",
  NULL
};


ARGUMENTS = {
  Argument ("file", "DICOM file", "the DICOM file to be scanned.", AllowMultiple).type_file (),
  Argument::End
};


OPTIONS = {
  Option ("all", "all DICOM fields", "print all DICOM fields."),
  Option ("csa", "Siemens CSA fields", "print all Siemens CSA fields"),
  Option::End
};


EXECUTE {
  File::Dicom::QuickScan reader;

  bool print_DICOM_fields = false;
  bool print_CSA_fields = false;

  if (get_options (0).size()) print_DICOM_fields = true;
  if (get_options (1).size()) print_CSA_fields = true;

  for (size_t n = 0; n < argument.size();  n++) {

    if (Path::is_dir (argument[n].get_string())) {
      Path::Dir* dir;
      try { dir = new Path::Dir (argument[n].get_string()); }
      catch (...) { throw Exception (std::string ("error opening folder \"") + argument[n].get_string() 
          + "\": " + strerror (errno)); }
      
      std::string entry;
      while ((entry = dir->read_name()).size()) {
        if (reader.read (Path::join (argument[n].get_string(), entry), print_DICOM_fields, print_CSA_fields))
          error ("error reading file \"" + reader.filename + "\"");
        else cout << reader << "\n";
      }
    }
    else if (reader.read (argument[n].get_string(), print_DICOM_fields, print_CSA_fields))
      error ("error reading file \"" + reader.filename + "\"");

    else if (!print_DICOM_fields) cout << reader << "\n";
  }

}
  
