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

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "output DICOM fields in human-readable format.";

  ARGUMENTS
  + Argument ("file", "the DICOM file to be scanned.")
  .allow_multiple().type_file ();

  OPTIONS
  + Option ("all", "print all DICOM fields.")
  + Option ("csa", "print all Siemens CSA fields");
}


void run ()
{
  File::Dicom::QuickScan reader;

  bool print_DICOM_fields = false;
  bool print_CSA_fields = false;

  if (get_options ("all").size())
    print_DICOM_fields = true;

  if (get_options ("csa").size())
    print_CSA_fields = true;

  for (size_t n = 0; n < argument.size();  n++) {

    if (Path::is_dir (argument[n])) {
      Path::Dir* dir;
      try {
        dir = new Path::Dir (argument[n]);
      }
      catch (...) {
        throw Exception ("error opening folder \"" + argument[n]
                         + "\": " + strerror (errno));
      }

      std::string entry;
      while ( (entry = dir->read_name()).size()) {
        if (reader.read (Path::join (argument[n], entry), print_DICOM_fields, print_CSA_fields))
          error ("error reading file \"" + reader.filename + "\"");
        else std::cout << reader << "\n";
      }
    }
    else if (reader.read (argument[n], print_DICOM_fields, print_CSA_fields))
      error ("error reading file \"" + reader.filename + "\"");

    else if (!print_DICOM_fields) std::cout << reader << "\n";
  }

}

