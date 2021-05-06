/* Copyright (c) 2008-2021 the MRtrix3 contributors.
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
#include "file/path.h"
#include "file/utils.h"
#include "file/ofstream.h"
#include "file/dicom/element.h"
#include "file/dicom/quick_scan.h"
#include "file/dicom/definitions.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Edit DICOM file in-place";

  DESCRIPTION
  + "Allows the modification or removal of specific DICOM tags, necessary for the "
    "purposes of anomysation";


  ARGUMENTS
  + Argument ("input", "the input DICOM file or folder to be edited.").type_text()
  + Argument ("output", "the output DICOM file or folded to be produced.").type_text();


  OPTIONS
  + Option ("anonymise", "remove any identifiable information, by replacing the following tags:\n"
      "- any tag with Value Representation PN will be replaced with 'anonymous'\n"
      "- tag (0010,0030) PatientBirthDate will be replaced with an empty string\n"
      "WARNING: there is no guarantee that this command will remove all identiable "
      "information, since such information may be contained in any number "
      "of private vendor-specific tags. You will need to double-check the "
      "results independently if you need to ensure anonymity.")

  + Option ("id", "replace all ID tags with string supplied. This consists of tags "
      "(0010, 0020) PatientID and (0010, 1000) OtherPatientIDs")
  +   Argument ("text").type_text()

  + Option ("delete", "remove all entries matching the specified group & element tags. "
      "if 'element' is specified as 'all', this will remove all entries with the matching group.").allow_multiple()
  +   Argument ("group")
  +   Argument ("element")

  + Option ("replace", "replace specific tag.").allow_multiple()
  +   Argument ("group")
  +   Argument ("element")
  +   Argument ("newvalue");
}


class Tag { NOMEMALIGN
  public:
    Tag (uint16_t group) :
      group (group), element (0), replace (false), groupwise_delete (true) { }
    Tag (uint16_t group, uint16_t element) :
      group (group), element (element), replace (false), groupwise_delete (false) { }
    Tag (uint16_t group, uint16_t element, const std::string& newvalue) :
      group (group), element (element), newvalue (newvalue), replace (true), groupwise_delete (false) { }
    const uint16_t group, element;
    const std::string newvalue;
    const bool replace, groupwise_delete;
};




inline std::string hex (uint16_t value)
{
  std::ostringstream hex;
  hex << std::hex << value;
  return hex.str();
}

inline uint16_t read_hex (const std::string& m)
{
  uint16_t value;
  std::istringstream hex (m);
  hex >> std::hex >> value;
  return value;
}







class DICOMEntry : public File::Dicom::Element
{
  public:
    uint8_t* address () const { return start; }
    size_t   header () const { return data-start; }
    void write_leadin (std::ostream& out) const {
      if (start > fmap->address())
        out.write ((const char*) fmap->address(), start-fmap->address());
    }
    void write_updated (std::ostream& out, const std::string& contents) const {
      out.write ((const char*) start, data-start);
      uint32_t nbytes = std::min<uint32_t> (size, contents.size());
      out.write ((const char*) contents.c_str(), nbytes);
      while (nbytes < size) {
        out.put (0);
        nbytes++;
      }
    }

    void write (std::ostream& out, const vector<Tag>& tags, const vector<uint16_t> VRs) const
    {
      for (size_t n = 0; n < VRs.size(); ++n) {
        if (VR == VRs[n]) {
          write_updated (out, "anonymous");
          return;
        }
      }
      for (size_t n = 0; n < tags.size(); ++n) {
        if (tags[n].group == group && tags[n].groupwise_delete)
          return;
        if (is (tags[n].group, tags[n].element)) {
          if (tags[n].replace)
            write_updated (out, tags[n].newvalue);
          return;
        }
      }
      out.write ((const char*) start, next-start);
    }
};






void process (const std::string& src, const std::string& dest, const vector<Tag>& tags, const vector<uint16_t> VRs)
{
  if (Path::is_dir (src)) {
    File::mkdir (dest, false);
    Path::Dir dir (src);
    std::string filename;
    while ((filename = dir.read_name()).size()) {
      try {
        process (Path::join (src, filename), Path::join (dest, filename), tags, VRs);
      }
      catch (Exception& E) {
        E.display (2);
      }
    }
    return;
  }

  DICOMEntry item;
  item.set (src, true, true);
  File::OFStream out (dest);

  item.write_leadin (out);
  while (item.read())
    item.write (out, tags, VRs);
}





void run ()
{
  vector<Tag> tags;
  vector<uint16_t> VRs;

  auto opt = get_options ("anonymise");
  if (opt.size()) {
    tags.push_back (Tag (0x0010U, 0x0030U, "")); // PatientBirthDate
    VRs.push_back (VR_PN);
  }


  opt = get_options ("delete");
  if (opt.size()) {
    for (size_t n = 0; n < opt.size(); ++n) {
      if (lowercase (opt[n][1]) == "all")
        tags.push_back (Tag (read_hex (opt[n][0])));
      else
        tags.push_back (Tag (read_hex (opt[n][0]), read_hex (opt[n][1])));
    }
  }

  opt = get_options ("replace");
  if (opt.size())
    for (size_t n = 0; n < opt.size(); ++n)
      tags.push_back (Tag (read_hex (opt[n][0]), read_hex (opt[n][1]), opt[n][2]));

  opt = get_options ("id");
  if (opt.size()) {
    std::string newid = opt[0][0];
    tags.push_back (Tag (0x0010U, 0x0020U, newid)); // PatientID
    tags.push_back (Tag (0x0010U, 0x1000U, newid)); // OtherPatientIDs
  }

  for (size_t n = 0; n < VRs.size(); ++n) {
    union __VR { uint16_t i; char c[2]; } VR;
    VR.i = VRs[n];
    INFO (std::string ("clearing entries with VR \"") + VR.c[1] + VR.c[0] + "\"");
  }
  for (size_t n = 0; n < tags.size(); ++n)
    INFO ("replacing tag (" + hex(tags[n].group) + "," + hex(tags[n].element) + ") with value \"" + tags[n].newvalue + "\"");

  process (argument[0], argument[1], tags, VRs);
}

