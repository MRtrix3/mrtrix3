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
#include "debug.h"
#include "file/path.h"
#include "file/dicom/element.h"
#include "file/dicom/quick_scan.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "output DICOM fields in human-readable format.";

  ARGUMENTS
  + Argument ("file", "the DICOM file to be scanned.").type_file ();

  OPTIONS
  + Option ("all", "print all DICOM fields.")

  + Option ("csa", "print all Siemens CSA fields")

  + Option ("force", "force full scan even if DICOM magic word is not present and file does not have .dcm extension.")

  + Option ("tag", "print field specified by the group & element tags supplied. "
      "Tags should be supplied as Hexadecimal (i.e. as they appear in the -all listing).")
  .allow_multiple()
  + Argument ("group")
  + Argument ("element");
}


class Tag {
  public:
    uint16_t group, element;
    std::string value;
};

inline uint16_t read_hex (const std::string& m)
{
  uint16_t value;
  std::istringstream hex (m);
  hex >> std::hex >> value;
  return value;
}

void run ()
{
  bool force_read = get_options ("force").size();

  Options opt = get_options("tag");
  if (opt.size()) {
    std::istringstream hex; 

    Tag tags[opt.size()];
    for (size_t n = 0; n < opt.size(); ++n) {
      tags[n].group = read_hex (opt[n][0]);
      tags[n].element = read_hex (opt[n][1]);
    }

    File::Dicom::Element item;
    item.set (argument[0], force_read);
    while (item.read()) {
      for (size_t n = 0; n < opt.size(); ++n) 
        if (item.is (tags[n].group, tags[n].element)) 
          tags[n].value = item.get_string()[0];
    }

    for (size_t n = 0; n < opt.size(); ++n) 
      std::cout << tags[n].value << "\n";

    return;
  }


  File::Dicom::QuickScan reader;

  if (reader.read (argument[0], get_options ("all").size(), get_options ("csa").size(), force_read))
    throw Exception ("error reading file \"" + reader.filename + "\"");

  if (!get_options ("all").size() && !get_options ("csa").size()) 
    std::cout << reader;
}

