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

#pragma once

#include <unordered_map>

#include "file/dicom/definitions.h"
#include "file/mmap.h"
#include "memory.h"
#include "raw.h"
#include "types.h"

namespace MR::File::Dicom {

class Sequence {
public:
  Sequence(uint16_t group, uint16_t element, uint8_t *end) : group(group), element(element), end(end) {}
  uint16_t group, element;
  uint8_t *end;

  bool is(uint16_t Group, uint16_t Element) const {
    if (group != Group)
      return false;
    return element == Element;
  }
};

class Date {
public:
  Date(std::string_view entry) : year(0), month(0), day(0) {
    if (entry.size() >= 8) {
      year = to<uint32_t>(entry.substr(0, 4));
      month = to<uint32_t>(entry.substr(4, 2));
      day = to<uint32_t>(entry.substr(6, 2));
    }
    if (year < 1000 || month > 12 || day > 31)
      throw Exception("Error converting string \"" + entry + "\" to date");
  }
  uint32_t year, month, day;
  friend std::ostream &operator<<(std::ostream &stream, const Date &item);
};

class Time {
public:
  Time(std::string_view entry) : Time() {
    if (entry.size() < 6)
      throw Exception("field \"" + entry + "\" is too short to be interpreted as a time");
    hour = to<uint32_t>(entry.substr(0, 2));
    minute = to<uint32_t>(entry.substr(2, 2));
    second = to<uint32_t>(entry.substr(4, 2));
    fraction = entry.size() > 6 ? to<default_type>(entry.substr(6)) : 0.0;
  }
  Time(default_type i) {
    if (i < 0.0)
      throw Exception("Error converting negative floating-point number to a time");
    hour = std::floor(i / 3600.0);
    i -= hour * 3600.0;
    if (hour >= 24)
      throw Exception("Error converting floating-point number to a time: Beyond 24 hours");
    minute = std::floor(i / 60.0);
    i -= minute * 60;
    second = std::floor(i);
    fraction = i - second;
  }
  Time() : hour(0), minute(0), second(0), fraction(0.0) {}
  operator default_type() const { return (hour * 3600.0 + minute * 60 + second + fraction); }
  Time operator-(const Time &t) const { return Time(static_cast<default_type>(*this) - static_cast<default_type>(t)); }
  uint32_t hour, minute, second;
  default_type fraction;
  friend std::ostream &operator<<(std::ostream &stream, const Time &item);
};

class Element {
public:
  typedef enum _Type { INVALID, INT, UINT, FLOAT, DATE, TIME, DATETIME, STRING, SEQ, OTHER } Type;
  static const std::unordered_map<Type, std::string> type_as_str;

  uint16_t group, element, VR;
  uint32_t size;
  uint8_t *data;
  std::vector<Sequence> parents;
  bool transfer_syntax_supported;

  void set(std::string_view filename, bool force_read = false, bool read_write = false);
  bool read();

  bool is(uint16_t Group, uint16_t Element) const {
    if (group != Group)
      return false;
    return element == Element;
  }

  std::string tag_name() const {
    if (dict.empty())
      init_dict();
    const char *s = dict[tag()]; // check_syntax off
    return (s == nullptr ? "" : s);
  }

  uint32_t tag() const {
    union __DICOM_group_element_pair__ {
      uint16_t s[2]; // check_syntax off
      uint32_t i;
    } val = {{
#if MRTRIX_BYTE_ORDER_BIG_ENDIAN
        group, element
#else
        element, group
#endif
    }};
    return val.i;
  }

  size_t offset(uint8_t *address) const { return address - fmap->address(); }
  bool is_big_endian() const { return is_BE; }
  bool is_new_sequence() const {
    return VR == VR_SQ || (group == group_data && element == element_data && size == undefined_length);
  }

  bool ignore_when_parsing() const;
  bool is_in_series_ref_sequence() const;

  Type type() const;
  std::vector<int32_t> get_int() const;
  std::vector<uint32_t> get_uint() const;
  std::vector<default_type> get_float() const;
  Date get_date() const;
  Time get_time() const;
  std::pair<Date, Time> get_datetime() const;
  std::vector<std::string> get_string() const;

  int32_t get_int(size_t idx, int32_t default_value = 0) const {
    auto v(get_int());
    return check_get(idx, v.size()) ? v[idx] : default_value;
  }
  uint32_t get_uint(size_t idx, uint32_t default_value = 0) const {
    auto v(get_uint());
    return check_get(idx, v.size()) ? v[idx] : default_value;
  }
  double get_float(size_t idx, double default_value = 0.0) const {
    auto v(get_float());
    return check_get(idx, v.size()) ? v[idx] : default_value;
  }
  std::string get_string(size_t idx, std::string default_value = std::string()) const {
    auto v(get_string());
    return check_get(idx, v.size()) ? v[idx] : default_value;
  }

  std::string as_string() const;

  size_t level() const { return parents.size(); }

  friend std::ostream &operator<<(std::ostream &stream, const Element &item);
  static std::string print_header() {
    return "TYPE  GRP  ELEM VR     SIZE   OFFSET   NAME                                   CONTENTS\n"
           "----- ---- ---- --  -------  -------   -------------------------------------  "
           "---------------------------------------\n";
  }

  template <typename VectorType> FORCE_INLINE void check_size(const VectorType v, size_t min_size = 1) {
    if (v.size() < min_size)
      error_in_check_size(min_size, v.size());
  }

protected:
  std::unique_ptr<File::MMap> fmap;
  void set_explicit_encoding();
  bool read_GR_EL();

  uint8_t *next;
  uint8_t *start;
  bool is_explicit, is_BE, is_transfer_syntax_BE;

  std::vector<uint8_t *> end_seq;

  static uint16_t get_VR_from_tag_name(std::string_view name) {
    union {
      std::array<char, 2> t;
      uint16_t i;
    } d = {{name[0], name[1]}};
    return ByteOrder::BE(d.i);
  }

  static std::unordered_map<uint32_t, const char *> dict;
  static void init_dict();

  bool check_get(size_t idx, size_t size) const {
    if (idx >= size) {
      error_in_get(idx);
      return false;
    }
    return true;
  }
  void error_in_get(size_t idx) const;
  void error_in_check_size(size_t min_size, size_t actual_size) const;
  void report_unknown_tag_with_implicit_syntax() const;
};

} // namespace MR::File::Dicom
