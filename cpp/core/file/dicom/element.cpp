/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include "file/dicom/element.h"

#include <cstring>
#include <iomanip>
#include <memory>

#include "debug.h"
#include "file/path.h"

namespace MR::File::Dicom {

std::ostream &operator<<(std::ostream &stream, const Date &item) {
  stream << item.year << "/" << std::setfill('0') << std::setw(2) << item.month << "/" << std::setfill('0')
         << std::setw(2) << item.day;
  return stream;
}

std::ostream &operator<<(std::ostream &stream, const Time &item) {
  stream << std::setfill('0') << std::setw(2) << item.hour << ":" << std::setfill('0') << std::setw(2) << item.minute
         << ":" << std::setfill('0') << std::setw(2) << item.second;
  if (item.fraction != 0.0)
    stream << str(item.fraction, 6).substr(1);
  return stream;
}

const std::unordered_map<Element::Type, std::string> Element::type_as_str{{Type::INVALID, "invalid"},
                                                                          {Type::INT, "integer"},
                                                                          {Type::UINT, "unsigned integer"},
                                                                          {Type::FLOAT, "floating-point"},
                                                                          {Type::DATE, "date"},
                                                                          {Type::TIME, "time"},
                                                                          {Type::STRING, "string"},
                                                                          {Type::SEQ, "sequence"},
                                                                          {Type::OTHER, "other"}};

void Element::set(const std::filesystem::path &filepath, bool force_read, bool read_write) {
  group = element = VR = 0;
  size = 0;
  start = data = next = nullptr;
  is_BE = is_transfer_syntax_BE = false;
  transfer_syntax_supported = true;
  parents.clear();

  fmap = std::make_unique<File::MMap>(filepath, read_write);

  if (fmap->size() < 256)
    throw Exception("\"" + fmap->path().string() + "\" is too small to be a valid DICOM file");

  next = fmap->address();

  if (memcmp(next + 128, "DICM", 4) != 0) {
    is_explicit = false;
    DEBUG("DICOM magic number not found in file \"" + fmap->path().string() + "\" - trying truncated format");
    if (!force_read)
      if (fmap->path().extension() != ".dcm")
        throw Exception("file \"" + fmap->path().string() + "\"" + //
                        " does not have the DICOM magic number or the .dcm extension - assuming not DICOM");
  } else
    next += 132;

  try {
    set_explicit_encoding();
  } catch (Exception &e) {
    fmap.reset();
    throw Exception(e, "\"" + fmap->path().string() + "\" is not a valid DICOM file");
  }
}

void Element::set_explicit_encoding() {
  assert(fmap);
  if (read_GR_EL())
    throw Exception("\"" + fmap->path().string() + "\" is too small to be DICOM");

  is_explicit = true;
  next = start;
  VR = ByteOrder::BE(*reinterpret_cast<uint16_t *>(start + 4));

  if ((VR == VR_OB) || (VR == VR_OW) || (VR == VR_OF) || (VR == VR_SQ) || (VR == VR_UN) || (VR == VR_AE) ||
      (VR == VR_AS) || (VR == VR_AT) || (VR == VR_CS) || (VR == VR_DA) || (VR == VR_DS) || (VR == VR_DT) ||
      (VR == VR_FD) || (VR == VR_FL) || (VR == VR_IS) || (VR == VR_LO) || (VR == VR_LT) || (VR == VR_PN) ||
      (VR == VR_SH) || (VR == VR_SL) || (VR == VR_SS) || (VR == VR_ST) || (VR == VR_TM) || (VR == VR_UI) ||
      (VR == VR_UL) || (VR == VR_US) || (VR == VR_UT))
    return;

  DEBUG("using implicit DICOM encoding");
  is_explicit = false;
}

bool Element::read_GR_EL() {
  group = element = VR = 0;
  size = 0;
  start = next;
  data = next = nullptr;

  if (start < fmap->address())
    throw Exception("invalid DICOM element");

  if (start + 8 > fmap->address() + fmap->size())
    return true;

  is_BE = is_transfer_syntax_BE;

  group = Raw::fetch_<uint16_t>(start, is_BE);

  if (group == group_byte_order_swapped) {
    if (!is_BE)
      throw Exception("invalid DICOM group ID " + str(group) + " in file \"" + fmap->path().string() + "\"");

    is_BE = false;
    group = group_byte_order;
  }
  element = Raw::fetch_<uint16_t>(start + 2, is_BE);

  return false;
}

bool Element::read() {
  if (read_GR_EL())
    return false;

  data = start + 8;
  if ((is_explicit && group != group_sequence) || group == group_byte_order) {

    // explicit encoding:
    VR = ByteOrder::BE(*reinterpret_cast<uint16_t *>(start + 4));
    if (VR == VR_OB || VR == VR_OW || VR == VR_OF || VR == VR_SQ || VR == VR_UN || VR == VR_UT) {
      size = Raw::fetch_<uint32_t>(start + 8, is_BE);
      data += 4;
    } else
      size = Raw::fetch_<uint16_t>(start + 6, is_BE);

    // try figuring out VR from dictionary if vendors haven't bothered
    // filling it in...
    if (VR == VR_UN) {
      const std::string name = tag_name();
      if (!name.empty())
        VR = get_VR_from_tag_name(name);
    }
  } else {

    // implicit encoding:
    const std::string name = tag_name();
    if (name.empty()) {
      DEBUG(printf("WARNING: unknown DICOM tag (%04X %04X) "
                   "with implicit encoding in file \"",
                   group,
                   element) +
            fmap->path().string() + "\"");
      VR = VR_UN;
    } else
      VR = get_VR_from_tag_name(name);
    size = Raw::fetch_<uint32_t>(start + 4, is_BE);
  }

  next = data;

  if (size == undefined_length) {
    if (VR != VR_SQ && !(group == group_sequence && element == element_sequence_item))
      INFO("undefined length used for DICOM tag " +            //
           (!tag_name().empty() ? tag_name().substr(2) : "") + //
           MR::printf("(%04X, %04X)", group, element) +        //
           " in file \"" + fmap->path().string() + "\"");
  } else if (next + size > fmap->address() + fmap->size())
    throw Exception("file \"" + fmap->path().string() + "\" is too small to contain DICOM elements specified");
  else {
    if ((size % 2) != 0U)
      DEBUG("WARNING: odd length (" + str(size) + ")" +         //
            " used for DICOM tag " +                            //
            (!tag_name().empty() ? tag_name().substr(2) : "") + //
            " (" + str(group) + ", " + str(element) + ")" +     //
            " in file \"" + fmap->path().string() + "");
    if (VR != VR_SQ) {
      if (group == group_sequence && element == element_sequence_item) {
        if (!parents.empty() && parents.back().group == group_data && parents.back().element == element_data)
          next += size;
      } else
        next += size;
    }
  }

  if (!parents.empty()) {
    if (group == group_sequence && element == element_sequence_delimitation_item) {
      parents.pop_back();
    } else { // Undefined length encoding
      while (!parents.empty() && (parents.back().end != nullptr) && data > parents.back().end)
        parents.pop_back();
    }
  }

  if (is_new_sequence())
    parents.emplace_back(group, element, size == undefined_length ? nullptr : data + size);

  switch (group) {
  case group_byte_order:
    switch (element) {
    case element_transfer_syntax_uid: {
      std::string data_as_string(reinterpret_cast<const char *>(data), size);
      data_as_string.erase(data_as_string.find_last_not_of('\0') + 1, std::string::npos);
      if (data_as_string == "1.2.840.10008.1.2.1") {
        is_BE = is_transfer_syntax_BE = false; // explicit VR Little Endian
        is_explicit = true;
      } else if (data_as_string == "1.2.840.10008.1.2.2") {
        is_BE = is_transfer_syntax_BE = true; // Explicit VR Big Endian
        is_explicit = true;
      } else if (data_as_string == "1.2.840.10008.1.2") {
        is_BE = is_transfer_syntax_BE = false; // Implicit VR Little Endian
        is_explicit = false;
      } else if (data_as_string == "1.2.840.10008.1.2.1.99") {
        throw Exception("DICOM deflated explicit VR little endian transfer syntax not supported");
      } else {
        transfer_syntax_supported = false;
        INFO("unsupported DICOM transfer syntax: \"" + data_as_string + "\" in file \"" + fmap->path().string() + "\"");
      }
    } break;
    default:
      break;
    }

    break;
  default:
    break;
  }

  return true;
}

bool Element::ignore_when_parsing() const {
  for (const auto &seq : parents) {
    // ignore anything within IconImageSequence:
    if (seq.is(0x0088u, 0x0200u))
      return true;
    // allow Philips PrivatePerFrameSq:
    if (seq.is(0x2005u, 0x140Fu))
      continue;
    // ignore anything within sequences with unknown (private) group:
    if ((seq.group & 1U) != 0U)
      return true;
  }

  return false;
}

bool Element::is_in_series_ref_sequence() const {
  // required to group together series exported using
  // Siemens XA10A in Interoperability mode
  for (const auto &seq : parents)
    if (seq.is(0x0008U, 0x1250U))
      return true;
  return false;
}

Element::Type Element::type() const {
  if (VR == 0U)
    return Element::Type::INVALID;
  if (VR == VR_FD || VR == VR_FL)
    return Element::Type::FLOAT;
  if (VR == VR_SL || VR == VR_SS)
    return Element::Type::INT;
  if (VR == VR_UL || VR == VR_US)
    return Element::Type::UINT;
  if (VR == VR_SQ)
    return Element::Type::SEQ;
  if (VR == VR_DA)
    return Element::Type::DATE;
  if (VR == VR_TM)
    return Element::Type::TIME;
  if (VR == VR_DT)
    return Element::Type::DATETIME;
  if (VR == VR_AE || VR == VR_AS || VR == VR_CS || VR == VR_DS || VR == VR_IS || VR == VR_LO || VR == VR_LT ||
      VR == VR_PN || VR == VR_SH || VR == VR_ST || VR == VR_UI || VR == VR_UT || VR == VR_AT)
    return Element::Type::STRING;
  return Element::Type::OTHER;
}

std::vector<int32_t> Element::get_int() const {
  std::vector<int32_t> V;
  if (VR == VR_SL)
    for (const std::byte *p = data; p < data + size; p += sizeof(int32_t))
      V.push_back(Raw::fetch_<int32_t>(p, is_BE));
  else if (VR == VR_SS)
    for (const std::byte *p = data; p < data + size; p += sizeof(int16_t))
      V.push_back(Raw::fetch_<int16_t>(p, is_BE));
  else if (VR == VR_IS) {
    auto strings = split(std::string(reinterpret_cast<const char *>(data), size), "\\", false);
    V.resize(strings.size());
    for (size_t n = 0; n < V.size(); n++)
      V[n] = to<int32_t>(strings[n]);
  } else
    report_unknown_tag_with_implicit_syntax();

  return V;
}

std::vector<uint32_t> Element::get_uint() const {
  std::vector<uint32_t> V;
  if (VR == VR_UL)
    for (const std::byte *p = data; p < data + size; p += sizeof(uint32_t))
      V.push_back(Raw::fetch_<uint32_t>(p, is_BE));
  else if (VR == VR_US)
    for (const std::byte *p = data; p < data + size; p += sizeof(uint16_t))
      V.push_back(Raw::fetch_<uint16_t>(p, is_BE));
  else if (VR == VR_IS) {
    auto strings = split(std::string(reinterpret_cast<const char *>(data), size), "\\", false);
    V.resize(strings.size());
    for (size_t n = 0; n < V.size(); n++)
      V[n] = to<uint32_t>(strings[n]);
  } else
    report_unknown_tag_with_implicit_syntax();
  return V;
}

std::vector<default_type> Element::get_float() const {
  std::vector<default_type> V;
  if (VR == VR_FD)
    for (const std::byte *p = data; p < data + size; p += sizeof(float64))
      V.push_back(Raw::fetch_<float64>(p, is_BE));
  else if (VR == VR_FL)
    for (const std::byte *p = data; p < data + size; p += sizeof(float32))
      V.push_back(Raw::fetch_<float32>(p, is_BE));
  else if (VR == VR_DS || VR == VR_IS) {
    auto strings = split(std::string(reinterpret_cast<const char *>(data), size), "\\", false);
    V.resize(strings.size());
    for (size_t n = 0; n < V.size(); n++)
      V[n] = to<default_type>(strings[n]);
  } else
    report_unknown_tag_with_implicit_syntax();
  return V;
}

Date Element::get_date() const {
  assert(type() == Element::Type::DATE);
  return Date(std::string(reinterpret_cast<const char *>(data), size));
}

Time Element::get_time() const {
  assert(type() == Element::Type::TIME);
  return Time(std::string(reinterpret_cast<const char *>(data), size));
}

std::pair<Date, Time> Element::get_datetime() const {
  assert(type() == Element::Type::DATETIME);
  if (size < 14)
    throw Exception("malformed DateTime entry");
  return {Date(std::string(reinterpret_cast<const char *>(data), 8)),
          Time(std::string(reinterpret_cast<const char *>(data + 8), std::min(size - 8, 13U)))};
}

std::vector<std::string> Element::get_string() const {
  if (VR == VR_AT)
    return {printf("%04X %04X", Raw::fetch_<uint16_t>(data, is_BE), Raw::fetch_<uint16_t>(data + 2, is_BE))};

  auto strings = split(std::string(reinterpret_cast<const char *>(data), size), "\\", false);
  for (auto &entry : strings)
    entry = strip(entry);
  return strings;
}

std::string Element::as_string() const {
  std::ostringstream out;
  try {
    switch (type()) {
    case Element::Type::INT:
      for (const auto &x : get_int())
        out << x << " ";
      return out.str();
    case Element::Type::UINT:
      for (const auto &x : get_uint())
        out << x << " ";
      return out.str();
    case Element::Type::FLOAT:
      for (const auto &x : get_float())
        out << x << " ";
      return out.str();
    case Element::Type::DATE:
      return str(get_date());
    case Element::Type::TIME:
      return str(get_time());
    case Element::Type::DATETIME:
      return str(get_datetime().first) + " " + str(get_datetime().second);
    case Element::Type::STRING:
      if (group == group_data && element == element_data) {
        return "(data)";
      } else {
        for (const auto &x : get_string())
          out << x << " ";
        return out.str();
      }
    case Element::Type::SEQ:
      return "";
    default:
      if (group != group_sequence || element != element_sequence_item)
        return "unknown data type";
    }
  } catch (Exception &e) {
    DEBUG("Error converting data at offset " + str(offset(start)) + " to " + type_as_str.at(type()) + " type: ");
    for (auto &s : e.description)
      DEBUG(s);
    return "invalid entry";
  }
  return "";
}

namespace {
template <class T> inline void print_vec(const std::vector<T> &V) {
  for (const auto &entry : V)
    fprintf(stdout, "%s ", str(entry).c_str());
}
} // namespace

void Element::error_in_get(size_t idx) const {
  const std::string name(tag_name());
  DEBUG("value not found for DICOM tag " +            //
        printf("%04X %04X ", group, element) +        //
        (name.empty() ? "unknown" : name.substr(2)) + //
        " (at index " + str(idx) + ")");              //
}

void Element::error_in_check_size(size_t min_size, size_t actual_size) const {
  const std::string name(tag_name());
  throw Exception("not enough items in for DICOM tag " +                              //
                  printf("%04X %04X ", group, element) +                              //
                  (name.empty() ? "unknown" : name.substr(2)) +                       //
                  " (expected " + str(min_size) + ", got " + str(actual_size) + ")"); //
}

void Element::report_unknown_tag_with_implicit_syntax() const {
  DEBUG(MR::printf("attempt to read data of unknown value representation "
                   "in DICOM implicit syntax for tag (%04X %04X); ignored",
                   group,
                   element));
}

std::ostream &operator<<(std::ostream &stream, const Element &item) {
  // return "TYPE  GROUP ELEMENT VR  SIZE  OFFSET  NAME                               CONTENTS";

  const std::string name(item.tag_name());
  stream << printf("[DCM] %04X %04X %c%c % 8u % 8llu ",
                   item.group,
                   item.element,
                   reinterpret_cast<const char *>(&item.VR)[1],
                   reinterpret_cast<const char *>(&item.VR)[0],
                   (item.size == undefined_length ? uint32_t(0) : item.size),
                   item.offset(item.start));

  std::string tmp;
  const size_t indent = item.level() - (item.VR == VR_SQ ? 1 : 0);
  for (size_t i = 0; i < indent; i++)
    tmp += "  ";
  if (item.is_new_sequence())
    tmp += "> ";
  else if (item.group == group_sequence && item.element == element_sequence_item)
    tmp += "- ";
  else
    tmp += "  ";
  tmp += (name.empty() ? "unknown" : name.substr(2));
  tmp.resize(40, ' ');
  stream << tmp << " " << item.as_string() << "\n";

  return stream;
}

} // namespace MR::File::Dicom
