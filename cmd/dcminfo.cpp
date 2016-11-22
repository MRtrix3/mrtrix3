/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */



#include "command.h"
#include "debug.h"
#include "file/path.h"
#include "file/dicom/element.h"
#include "file/dicom/quick_scan.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  DESCRIPTION
  + "output DICOM fields in human-readable format.";

  ARGUMENTS
  + Argument ("file", "the DICOM file to be scanned.").type_file_in();

  OPTIONS
  + Option ("all", "print all DICOM fields.")

  + Option ("csa", "print all Siemens CSA fields")

  + Option ("tag", "print field specified by the group & element tags supplied. "
      "Tags should be supplied as Hexadecimal (i.e. as they appear in the -all listing).")
  .allow_multiple()
  + Argument ("group").type_text()
  + Argument ("element").type_text();
}


class Tag { NOMEMALIGN
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
  auto opt = get_options("tag");
  if (opt.size()) {
    std::istringstream hex;

    std::vector<Tag> tags (opt.size());
    for (size_t n = 0; n < opt.size(); ++n) {
      tags[n].group = read_hex (opt[n][0]);
      tags[n].element = read_hex (opt[n][1]);
    }

    File::Dicom::Element item;
    item.set (argument[0], true);
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

  if (get_options("all").size())
    print (File::Dicom::Element::print_header());

  if (reader.read (argument[0], get_options ("all").size(), get_options ("csa").size(), true))
    throw Exception ("error reading file \"" + reader.filename + "\"");

  if (!get_options ("all").size() && !get_options ("csa").size())
    std::cout << reader;
}

