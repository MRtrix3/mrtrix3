/* Copyright (c) 2008-2025 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "command.h"
#include "debug.h"
#include "file/dicom/element.h"
#include "file/dicom/quick_scan.h"
#include "file/path.h"

#include <filesystem>

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Output DICOM fields in human-readable format";

  ARGUMENTS
  + Argument ("file", "the DICOM file to be scanned.").type_file_in();

  OPTIONS
  + Option ("all", "print all DICOM fields.")

  + Option ("csa", "print all Siemens CSA fields"
                   " (excluding Phoenix unless requested)")
  + Option ("phoenix", "print Siemens Phoenix protocol information")

  + Option ("tag", "print field specified by the group & element tags supplied."
                   " Tags should be supplied as Hexadecimal"
                   " (i.e. as they appear in the -all listing).").allow_multiple()
    + Argument ("group").type_text()
    + Argument ("element").type_text();

}
// clang-format on

class Tag {
public:
  uint16_t group, element;
  std::string value;
};

inline uint16_t read_hex(const std::string &m) {
  uint16_t value;
  std::istringstream hex(m);
  hex >> std::hex >> value;
  return value;
}

void run() {
  const std::filesystem::path input_path{argument[0]};
  auto opt = get_options("tag");
  if (!opt.empty()) {
    std::istringstream hex;

    std::vector<Tag> tags(opt.size());
    for (size_t n = 0; n < opt.size(); ++n) {
      tags[n].group = read_hex(opt[n][0]);
      tags[n].element = read_hex(opt[n][1]);
    }

    File::Dicom::Element item;
    item.set(argument[0], true);
    while (item.read()) {
      for (size_t n = 0; n < opt.size(); ++n)
        if (item.is(tags[n].group, tags[n].element))
          std::cout << MR::printf("[%04X,%04X] ", tags[n].group, tags[n].element) << item.as_string() << "\n";
    }

    return;
  }

  File::Dicom::QuickScan reader;

  const bool all = !get_options("all").empty();
  const bool csa = !get_options("csa").empty();
  const bool phoenix = !get_options("phoenix").empty();

  if (all)
    print(File::Dicom::Element::print_header());

  if (reader.read(argument[0], all, csa, phoenix, true))
    throw Exception("error reading file \"" + reader.filename + "\"");

  if (!all && !csa && !phoenix)
    std::cout << reader;
}
