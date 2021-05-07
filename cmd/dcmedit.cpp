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
#include "progressbar.h"
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
    "purposes of anomysation. In most cases, the -anonymise option will take all the "
    "necessary steps."

  + "When running with -anonymise, this tool will remove all private groups as "
    "mandated by the DICOM standard. This may prevent correct interpretation of "
    "Siemens mosaic images. In such cases, you can preverse the relevant information "
    "by adding the '-preserve 0029 1010' option.";


  ARGUMENTS
  + Argument ("input", "the input DICOM file or folder to be edited.").type_text()
  + Argument ("output", "the output DICOM file or folded to be produced.").type_text();


  OPTIONS
  + Option ("anonymise", "remove any identifiable information, according to the "
      "DICOM Basic Profile as described in part 15, Chapter E.")

  + Option ("delete", "remove all entries matching the specified group & element tags. "
      "if 'element' is specified as 'all', this will remove all entries with the matching group.").allow_multiple()
  +   Argument ("group")
  +   Argument ("element")

  + Option ("zero", "zero all entries matching the specified group & element tags. "
      "if 'element' is specified as 'all', this will remove all entries with the matching group.").allow_multiple()
  +   Argument ("group")
  +   Argument ("element")

  + Option ("replace", "replace specific tag.").allow_multiple()
  +   Argument ("group")
  +   Argument ("element")
  +   Argument ("newvalue")

  + Option ("preserve", "preserve all entries matching the specified group & element tags. "
      "This is useful to prevent deletion or modification of tags otherwise selected with the -anonymise option. "
      "if 'element' is specified as 'all', this will preserve all entries with the matching group.").allow_multiple()
  +   Argument ("group")
  +   Argument ("element");
}







inline uint16_t read_hex (const std::string& m)
{
  uint16_t value;
  std::istringstream hex (m);
  hex >> std::hex >> value;
  return value;
}





class Tag { NOMEMALIGN
  public:
    Tag (uint16_t group, uint16_t element = 0,
        const std::string& newvalue = std::string(),
        bool replace = true, bool groupwise_delete = false) :
      group (group),
      element (element),
      newvalue (newvalue),
      replace (replace),
      groupwise_delete (groupwise_delete) { }
    const uint16_t group, element;
    const std::string newvalue;
    const bool replace, groupwise_delete;

    Tag groupwise () const { return Tag (group, 0, newvalue, replace, true); }
    Tag remove () const { return Tag (group, element, std::string(), false, groupwise_delete); }
};








class DICOMEntry : public File::Dicom::Element
{
  public:
    uint8_t* address () const { return start; }
    size_t   header () const { return data-start; }
    void write_leadin (std::ostream& out) const {
      if (start > fmap->address())
        out.write ((const char*) fmap->address(), start-fmap->address());
    }

    //! write entry unmodified:
    void write (std::ostream& out) const {
      out.write ((const char*) start, next-start);
    }


    //! write entry with updated value:
    void write (std::ostream& out, const std::string& contents) const {
      out.write ((const char*) start, data-start);
      uint32_t nbytes = std::min<uint32_t> (size, contents.size());
      out.write ((const char*) contents.c_str(), nbytes);
      while (nbytes < size) {
        out.put (0);
        nbytes++;
      }
    }

};


//! if entry is preserved, write out unmodified and return true
inline bool preserve (std::ostream& out, const vector<Tag>& tags, const DICOMEntry& item)
{
  for (const auto& tag : tags) {
    if (tag.group == item.group && ( tag.groupwise_delete || tag.element == item.element )) {
      item.write (out);
      return true;
    }
  }

  return false;
}

//! if entry is to be modified, write out modified version (unless removed) and return true
inline bool modify (std::ostream& out, const vector<Tag>& tags, const DICOMEntry& item)
{
  for (const auto& tag : tags) {
    if (tag.group == item.group) {
      if (tag.groupwise_delete)
        return true;
      if (tag.element == item.element ) {
        if (tag.replace)
          item.write (out, tag.newvalue);
        return true;
      }
    }
  }

  return false;
}



//! process path, recursing if path is a directory
void process (ProgressBar& progress, const std::string& src, const std::string& dest,
    const vector<Tag>& tags, const vector<Tag>& preserve_tags, bool remove_odd_groups)
{
  if (Path::is_dir (src)) {
    File::mkdir (dest, false);
    Path::Dir dir (src);
    std::string filename;
    while ((filename = dir.read_name()).size()) {
      try {
        process (progress,
            Path::join (src, filename), Path::join (dest, filename),
            tags, preserve_tags, remove_odd_groups);
      }
      catch (Exception& E) {
        E.display (2);
      }
    }
    return;
  }

  progress.set_text ("editing file \"" + src + "\"");
  progress++;

  DICOMEntry item;
  item.set (src, true, true);
  File::OFStream out (dest);

  item.write_leadin (out);
  while (item.read()) {
    if (preserve (out, preserve_tags, item))
      continue;

    if (remove_odd_groups && (item.group&0x0001U))
      continue;

    if (modify (out, tags, item))
      continue;

    item.write (out);
  }
}





//! set up all tags that need to be edited for compliant anonymisation
void init_anonymise (vector<Tag>& tags);







void run ()
{
  vector<Tag> tags;
  bool remove_odd_groups = false;

  auto opt = get_options ("anonymise");
  if (opt.size()) {
    init_anonymise (tags);
    remove_odd_groups = true;
  }

  opt = get_options ("delete");
  for (const auto& o : opt) {
    if (lowercase (o[1]) == "all")
      tags.push_back (Tag (read_hex(o[0])).groupwise().remove());
    else
      tags.push_back (Tag (read_hex(o[0]), read_hex(o[1])).remove());
  }

  opt = get_options ("zero");
  for (const auto& o : opt) {
    if (lowercase (o[1]) == "all")
      tags.push_back (Tag (read_hex(o[0])).groupwise());
    else
      tags.push_back (Tag (read_hex(o[0]), read_hex(o[1])));
  }

  opt = get_options ("replace");
  if (opt.size())
    for (const auto& o : opt)
      tags.push_back (Tag (read_hex(o[0]), read_hex(o[1]), o[2]));

  vector<Tag> preserve_tags;
  opt = get_options ("preserve");
  for (const auto& o : opt) {
    if (lowercase (o[1]) == "all")
      preserve_tags.push_back (Tag (read_hex(o[0])).groupwise());
    else
      preserve_tags.push_back (Tag (read_hex(o[0]), read_hex(o[1])));
  }

  for (const auto& tag : tags) {
    std::string text = tag.replace ? "replacing" : "removing";
    text += " tag";
    if (tag.groupwise_delete)
      text += "s with group " + MR::printf ("0x%.04X", tag.group);
    else
      text += " (" + MR::printf("0x%.04X", tag.group) + "," + MR::printf("0x%.04X", tag.element) + ")";
    if (tag.replace)
      text += " with value \"" + tag.newvalue + "\"";
    INFO (text);
  }

  for (const auto& tag : preserve_tags) {
    std::string text = "preserving tag";
    if (tag.groupwise_delete)
      text += "s with group " + MR::printf ("0x%.04X", tag.group);
    else
      text += " (" + MR::printf("0x%.04X", tag.group) + "," + MR::printf("0x%.04X", tag.element) + ")";
    INFO (text);
  }

  ProgressBar progress ("updating DICOM files");
  process (progress, argument[0], argument[1], tags, preserve_tags, remove_odd_groups);
}











void init_anonymise (vector<Tag>& tags)
{
  uint16_t dummy[][2] = {
    { 0x0012, 0x0010 },
    { 0x0012, 0x0020 },
    { 0x0012, 0x0040 },
    { 0x0012, 0x0042 },
    { 0x0012, 0x0081 },
    { 0x0018, 0x11BB },
    { 0x0018, 0x9367 },
    { 0x0018, 0x9369 },
    { 0x0018, 0x936A },
    { 0x0018, 0x9371 },
    { 0x0034, 0x0001 },
    { 0x0034, 0x0002 },
    { 0x0034, 0x0005 },
    { 0x0034, 0x0007 },
    { 0x003A, 0x0314 },
    { 0x0040, 0x0512 },
    { 0x0040, 0x0551 },
    { 0x0040, 0x1101 },
    { 0x0040, 0xA027 },
    { 0x0040, 0xA073 },
    { 0x0040, 0xA075 },
    { 0x0040, 0xA123 },
    { 0x0040, 0xA730 },
    { 0x0070, 0x0001 },
    { 0x3006, 0x0002 },
    { 0x300A, 0x0002 },
    { 0x300A, 0x0608 },
    { 0x300A, 0x0619 },
    { 0x300A, 0x0623 },
    { 0x300A, 0x062A },
    { 0x300A, 0x067C },
    { 0x300A, 0x0734 },
    { 0x300A, 0x0736 },
    { 0x300A, 0x073A },
    { 0x300A, 0x0741 },
    { 0x300A, 0x0742 },
    { 0x300A, 0x0760 },
    { 0x300A, 0x0783 },
    { 0x3010, 0x002D },
    { 0x3010, 0x0033 },
    { 0x3010, 0x0034 },
    { 0x3010, 0x0035 },
    { 0x3010, 0x0038 },
    { 0x3010, 0x0054 },
    { 0x3010, 0x0077 }
  };


  uint16_t replace_uid[][2] = {
    { 0x0000, 0x1001 },
    { 0x0002, 0x0003 },
    { 0x0004, 0x1511 },
    { 0x0008, 0x0014 },
    { 0x0008, 0x0018 },
    { 0x0008, 0x0058 },
    { 0x0008, 0x1155 },
    { 0x0008, 0x1195 },
    { 0x0008, 0x3010 },
    { 0x0018, 0x1002 },
    { 0x0018, 0x100B },
    { 0x0018, 0x2042 },
    { 0x0020, 0x000D },
    { 0x0020, 0x000E },
    { 0x0020, 0x0052 },
    { 0x0020, 0x0200 },
    { 0x0020, 0x9161 },
    { 0x0020, 0x9164 },
    { 0x0028, 0x1199 },
    { 0x0028, 0x1214 },
    { 0x003A, 0x0310 },
    { 0x0040, 0x0554 },
    { 0x0040, 0x4023 },
    { 0x0040, 0xA124 },
    { 0x0040, 0xA171 },
    { 0x0040, 0xA172 },
    { 0x0040, 0xA402 },
    { 0x0040, 0xDB0C },
    { 0x0040, 0xDB0D },
    { 0x0062, 0x0021 },
    { 0x0070, 0x031A },
    { 0x0070, 0x1101 },
    { 0x0070, 0x1102 },
    { 0x0088, 0x0140 },
    { 0x0400, 0x0100 },
    { 0x3006, 0x0024 },
    { 0x3006, 0x00C2 },
    { 0x300A, 0x0013 },
    { 0x300A, 0x0083 },
    { 0x300A, 0x0609 },
    { 0x300A, 0x0650 },
    { 0x300A, 0x0700 },
    { 0x3010, 0x0006 },
    { 0x3010, 0x000B },
    { 0x3010, 0x0013 },
    { 0x3010, 0x0015 },
    { 0x3010, 0x0031 },
    { 0x3010, 0x003B },
    { 0x3010, 0x006E },
    { 0x3010, 0x006F }
  };

  uint16_t remove[][2] = {
    { 0x0000, 0x1000 },
    { 0x0008, 0x0015 },
    { 0x0008, 0x0021 },
    { 0x0008, 0x0022 },
    { 0x0008, 0x0024 },
    { 0x0008, 0x0025 },
    { 0x0008, 0x002A },
    { 0x0008, 0x0031 },
    { 0x0008, 0x0032 },
    { 0x0008, 0x0034 },
    { 0x0008, 0x0035 },
    { 0x0008, 0x0080 },
    { 0x0008, 0x0081 },
    { 0x0008, 0x0082 },
    { 0x0008, 0x0092 },
    { 0x0008, 0x0094 },
    { 0x0008, 0x0096 },
    { 0x0008, 0x009D },
    { 0x0008, 0x0201 },
    { 0x0008, 0x1010 },
    { 0x0008, 0x1030 },
    { 0x0008, 0x103E },
    { 0x0008, 0x1040 },
    { 0x0008, 0x1041 },
    { 0x0008, 0x1048 },
    { 0x0008, 0x1049 },
    { 0x0008, 0x1050 },
    { 0x0008, 0x1052 },
    { 0x0008, 0x1060 },
    { 0x0008, 0x1062 },
    { 0x0008, 0x1070 },
    { 0x0008, 0x1072 },
    { 0x0008, 0x1080 },
    { 0x0008, 0x1084 },
    { 0x0008, 0x1110 },
    { 0x0008, 0x1111 },
    { 0x0008, 0x1120 },
    { 0x0008, 0x1140 },
    { 0x0008, 0x2111 },
    { 0x0008, 0x2112 },
    { 0x0008, 0x4000 },
    { 0x0010, 0x0021 },
    { 0x0010, 0x0032 },
    { 0x0010, 0x0050 },
    { 0x0010, 0x0101 },
    { 0x0010, 0x0102 },
    { 0x0010, 0x1000 },
    { 0x0010, 0x1001 },
    { 0x0010, 0x1002 },
    { 0x0010, 0x1005 },
    { 0x0010, 0x1010 },
    { 0x0010, 0x1020 },
    { 0x0010, 0x1030 },
    { 0x0010, 0x1040 },
    { 0x0010, 0x1050 },
    { 0x0010, 0x1060 },
    { 0x0010, 0x1080 },
    { 0x0010, 0x1081 },
    { 0x0010, 0x1090 },
    { 0x0010, 0x1100 },
    { 0x0010, 0x2000 },
    { 0x0010, 0x2110 },
    { 0x0010, 0x2150 },
    { 0x0010, 0x2152 },
    { 0x0010, 0x2154 },
    { 0x0010, 0x2155 },
    { 0x0010, 0x2160 },
    { 0x0010, 0x2180 },
    { 0x0010, 0x21A0 },
    { 0x0010, 0x21B0 },
    { 0x0010, 0x21C0 },
    { 0x0010, 0x21D0 },
    { 0x0010, 0x21F0 },
    { 0x0010, 0x2203 },
    { 0x0010, 0x2297 },
    { 0x0010, 0x2299 },
    { 0x0010, 0x4000 },
    { 0x0012, 0x0051 },
    { 0x0012, 0x0071 },
    { 0x0012, 0x0072 },
    { 0x0012, 0x0082 },
    { 0x0016, 0x002B },
    { 0x0016, 0x004B },
    { 0x0016, 0x004D },
    { 0x0016, 0x004E },
    { 0x0016, 0x004F },
    { 0x0016, 0x0050 },
    { 0x0016, 0x0051 },
    { 0x0016, 0x0070 },
    { 0x0016, 0x0071 },
    { 0x0016, 0x0072 },
    { 0x0016, 0x0073 },
    { 0x0016, 0x0074 },
    { 0x0016, 0x0075 },
    { 0x0016, 0x0076 },
    { 0x0016, 0x0077 },
    { 0x0016, 0x0078 },
    { 0x0016, 0x0079 },
    { 0x0016, 0x007A },
    { 0x0016, 0x007B },
    { 0x0016, 0x007C },
    { 0x0016, 0x007D },
    { 0x0016, 0x007E },
    { 0x0016, 0x007F },
    { 0x0016, 0x0080 },
    { 0x0016, 0x0081 },
    { 0x0016, 0x0082 },
    { 0x0016, 0x0083 },
    { 0x0016, 0x0084 },
    { 0x0016, 0x0085 },
    { 0x0016, 0x0086 },
    { 0x0016, 0x0087 },
    { 0x0016, 0x0088 },
    { 0x0016, 0x0089 },
    { 0x0016, 0x008A },
    { 0x0016, 0x008B },
    { 0x0016, 0x008C },
    { 0x0016, 0x008D },
    { 0x0016, 0x008E },
    { 0x0018, 0x1000 },
    { 0x0018, 0x1004 },
    { 0x0018, 0x1005 },
    { 0x0018, 0x1007 },
    { 0x0018, 0x1008 },
    { 0x0018, 0x1009 },
    { 0x0018, 0x100A },
    { 0x0018, 0x1030 },
    { 0x0018, 0x1400 },
    { 0x0018, 0x4000 },
    { 0x0018, 0x5011 },
    { 0x0018, 0x700A },
    { 0x0018, 0x9185 },
    { 0x0018, 0x9373 },
    { 0x0018, 0x937B },
    { 0x0018, 0x937F },
    { 0x0018, 0x9424 },
    { 0x0018, 0x9516 },
    { 0x0018, 0x9517 },
    { 0x0018, 0x9937 },
    { 0x0018, 0xA003 },
    { 0x0020, 0x3401 },
    { 0x0020, 0x3406 },
    { 0x0020, 0x4000 },
    { 0x0020, 0x9158 },
    { 0x0028, 0x4000 },
    { 0x0032, 0x0012 },
    { 0x0032, 0x1020 },
    { 0x0032, 0x1021 },
    { 0x0032, 0x1030 },
    { 0x0032, 0x1032 },
    { 0x0032, 0x1033 },
    { 0x0032, 0x1060 },
    { 0x0032, 0x1066 },
    { 0x0032, 0x1067 },
    { 0x0032, 0x1070 },
    { 0x0032, 0x4000 },
    { 0x0038, 0x0004 },
    { 0x0038, 0x0010 },
    { 0x0038, 0x0011 },
    { 0x0038, 0x0014 },
    { 0x0038, 0x001E },
    { 0x0038, 0x0020 },
    { 0x0038, 0x0021 },
    { 0x0038, 0x0040 },
    { 0x0038, 0x0050 },
    { 0x0038, 0x0060 },
    { 0x0038, 0x0061 },
    { 0x0038, 0x0062 },
    { 0x0038, 0x0064 },
    { 0x0038, 0x0300 },
    { 0x0038, 0x0400 },
    { 0x0038, 0x0500 },
    { 0x0038, 0x4000 },
    { 0x0040, 0x0001 },
    { 0x0040, 0x0002 },
    { 0x0040, 0x0003 },
    { 0x0040, 0x0004 },
    { 0x0040, 0x0005 },
    { 0x0040, 0x0006 },
    { 0x0040, 0x0007 },
    { 0x0040, 0x0009 },
    { 0x0040, 0x000B },
    { 0x0040, 0x0010 },
    { 0x0040, 0x0011 },
    { 0x0040, 0x0012 },
    { 0x0040, 0x0241 },
    { 0x0040, 0x0242 },
    { 0x0040, 0x0243 },
    { 0x0040, 0x0244 },
    { 0x0040, 0x0245 },
    { 0x0040, 0x0250 },
    { 0x0040, 0x0251 },
    { 0x0040, 0x0253 },
    { 0x0040, 0x0254 },
    { 0x0040, 0x0275 },
    { 0x0040, 0x0280 },
    { 0x0040, 0x0310 },
    { 0x0040, 0x050A },
    { 0x0040, 0x051A },
    { 0x0040, 0x0555 },
    { 0x0040, 0x0600 },
    { 0x0040, 0x0602 },
    { 0x0040, 0x06FA },
    { 0x0040, 0x1001 },
    { 0x0040, 0x1002 },
    { 0x0040, 0x1004 },
    { 0x0040, 0x1005 },
    { 0x0040, 0x100A },
    { 0x0040, 0x1010 },
    { 0x0040, 0x1011 },
    { 0x0040, 0x1102 },
    { 0x0040, 0x1103 },
    { 0x0040, 0x1104 },
    { 0x0040, 0x1400 },
    { 0x0040, 0x2001 },
    { 0x0040, 0x2008 },
    { 0x0040, 0x2009 },
    { 0x0040, 0x2010 },
    { 0x0040, 0x2011 },
    { 0x0040, 0x2400 },
    { 0x0040, 0x3001 },
    { 0x0040, 0x4005 },
    { 0x0040, 0x4008 },
    { 0x0040, 0x4010 },
    { 0x0040, 0x4011 },
    { 0x0040, 0x4025 },
    { 0x0040, 0x4027 },
    { 0x0040, 0x4028 },
    { 0x0040, 0x4030 },
    { 0x0040, 0x4034 },
    { 0x0040, 0x4035 },
    { 0x0040, 0x4036 },
    { 0x0040, 0x4037 },
    { 0x0040, 0x4050 },
    { 0x0040, 0x4051 },
    { 0x0040, 0x4052 },
    { 0x0040, 0xA078 },
    { 0x0040, 0xA07A },
    { 0x0040, 0xA07C },
    { 0x0040, 0xA192 },
    { 0x0040, 0xA193 },
    { 0x0040, 0xA307 },
    { 0x0040, 0xA352 },
    { 0x0040, 0xA353 },
    { 0x0040, 0xA354 },
    { 0x0040, 0xA358 },
    { 0x0050, 0x001B },
    { 0x0050, 0x0020 },
    { 0x0050, 0x0021 },
    { 0x0070, 0x0086 },
    { 0x0088, 0x0200 },
    { 0x0088, 0x0904 },
    { 0x0088, 0x0906 },
    { 0x0088, 0x0910 },
    { 0x0088, 0x0912 },
    { 0x0400, 0x0402 },
    { 0x0400, 0x0403 },
    { 0x0400, 0x0404 },
    { 0x0400, 0x0550 },
    { 0x0400, 0x0551 },
    { 0x0400, 0x0552 },
    { 0x0400, 0x0561 },
    { 0x0400, 0x0600 },
    { 0x2030, 0x0020 },
    { 0x2200, 0x0002 },
    { 0x2200, 0x0005 },
    { 0x3006, 0x0004 },
    { 0x3006, 0x0006 },
    { 0x3006, 0x0028 },
    { 0x3006, 0x0038 },
    { 0x3006, 0x0085 },
    { 0x3006, 0x0088 },
    { 0x3008, 0x0054 },
    { 0x3008, 0x0056 },
    { 0x3008, 0x0105 },
    { 0x3008, 0x0250 },
    { 0x3008, 0x0251 },
    { 0x300A, 0x0003 },
    { 0x300A, 0x0004 },
    { 0x300A, 0x0006 },
    { 0x300A, 0x0007 },
    { 0x300A, 0x000E },
    { 0x300A, 0x0016 },
    { 0x300A, 0x0072 },
    { 0x300A, 0x00B2 },
    { 0x300A, 0x00C3 },
    { 0x300A, 0x00DD },
    { 0x300A, 0x0196 },
    { 0x300A, 0x01A6 },
    { 0x300A, 0x01B2 },
    { 0x300A, 0x0216 },
    { 0x300A, 0x02EB },
    { 0x300A, 0x0676 },
    { 0x300C, 0x0113 },
    { 0x300E, 0x0008 },
    { 0x3010, 0x0036 },
    { 0x3010, 0x0037 },
    { 0x3010, 0x004C },
    { 0x3010, 0x004D },
    { 0x3010, 0x0056 },
    { 0x3010, 0x0061 },
    { 0x4000, 0x0010 },
    { 0x4000, 0x4000 },
    { 0x4008, 0x0042 },
    { 0x4008, 0x0102 },
    { 0x4008, 0x010A },
    { 0x4008, 0x010B },
    { 0x4008, 0x010C },
    { 0x4008, 0x0111 },
    { 0x4008, 0x0114 },
    { 0x4008, 0x0115 },
    { 0x4008, 0x0118 },
    { 0x4008, 0x0119 },
    { 0x4008, 0x011A },
    { 0x4008, 0x0202 },
    { 0x4008, 0x0300 },
    { 0x4008, 0x4000 },
    { 0xFFFA, 0xFFFA },
    { 0xFFFC, 0xFFFC }
  };

  uint16_t zero[][2] = {
    { 0x0008, 0x0020 },
    { 0x0008, 0x0023 },
    { 0x0008, 0x0030 },
    { 0x0008, 0x0033 },
    { 0x0008, 0x0050 },
    { 0x0008, 0x0090 },
    { 0x0008, 0x009C },
    { 0x0010, 0x0010 },
    { 0x0010, 0x0020 },
    { 0x0010, 0x0030 },
    { 0x0010, 0x0040 },
    { 0x0012, 0x0021 },
    { 0x0012, 0x0030 },
    { 0x0012, 0x0031 },
    { 0x0012, 0x0050 },
    { 0x0012, 0x0060 },
    { 0x0018, 0x0010 },
    { 0x0020, 0x0010 },
    { 0x0040, 0x0513 },
    { 0x0040, 0x0562 },
    { 0x0040, 0x0610 },
    { 0x0040, 0x2016 },
    { 0x0040, 0x2017 },
    { 0x0040, 0xA088 },
    { 0x0070, 0x0084 },
    { 0x3006, 0x0008 },
    { 0x3006, 0x0009 },
    { 0x3006, 0x0026 },
    { 0x3006, 0x00A6 },
    { 0x300A, 0x0611 },
    { 0x300A, 0x0615 },
    { 0x300A, 0x067D },
    { 0x3010, 0x000F },
    { 0x3010, 0x0017 },
    { 0x3010, 0x001B },
    { 0x3010, 0x0043 },
    { 0x3010, 0x005A },
    { 0x3010, 0x005C },
    { 0x3010, 0x007A },
    { 0x3010, 0x007B },
    { 0x3010, 0x007F },
    { 0x3010, 0x0081 }
  };


  for (const auto x : dummy)
    tags.push_back (Tag (x[0], x[1], "anonymous"));
  for (const auto x : replace_uid)
    tags.push_back (Tag (x[0], x[1], "12345678"));
  for (const auto x : remove)
    tags.push_back (Tag (x[0], x[1]).remove());
  for (const auto x : zero)
    tags.push_back (Tag (x[0], x[1]));


  /* TODO: not sure how to handle these cleanly:
     { 0x50xx, 0xxxxx }	X
     { 0x60xx, 0x3000 }	X
     { 0x60xx, 0x4000 }	X
     */
}


