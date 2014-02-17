/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#include "command.h"
#include "debug.h"
#include "file/path.h"
#include "file/dicom/element.h"
#include "file/dicom/quick_scan.h"


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
  Options opt = get_options("tag");
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

  if (reader.read (argument[0], get_options ("all").size(), get_options ("csa").size(), true))
    throw Exception ("error reading file \"" + reader.filename + "\"");

  if (!get_options ("all").size() && !get_options ("csa").size())
    std::cout << reader;
}

