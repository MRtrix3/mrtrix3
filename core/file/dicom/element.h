/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __file_dicom_element_h__
#define __file_dicom_element_h__

#include <unordered_map>

#include "memory.h"
#include "raw.h"
#include "types.h"
#include "file/mmap.h"
#include "file/dicom/definitions.h"

namespace MR {
  namespace File {
    namespace Dicom {


      class Sequence { NOMEMALIGN
        public:
          Sequence (uint16_t group, uint16_t element, uint8_t* end) : group (group), element (element), end (end) { }
          uint16_t group, element;
          uint8_t* end;
      };

      class Date { NOMEMALIGN
        public:
          Date (const std::string& entry) :
              year  (to<uint32_t> (entry.substr (0, 4))),
              month (to<uint32_t> (entry.substr (4, 2))),
              day   (to<uint32_t> (entry.substr (6, 2)))
          {
            if (year < 1000 || month > 12 || day > 31)
              throw Exception ("Error converting string \"" + entry + "\" to date");
          }
          uint32_t year, month, day;
          friend std::ostream& operator<< (std::ostream& stream, const Date& item);
      };

      class Time { NOMEMALIGN
        public:
          Time (const std::string& entry) :
              hour     (to<uint32_t> (entry.substr (0, 2))),
              minute   (to<uint32_t> (entry.substr (2, 2))),
              second   (to<uint32_t> (entry.substr (4, 2))),
              fraction (entry.size() > 6 ? to<default_type> (entry.substr (6)) : 0.0) { }
          Time (default_type i)
          {
            if (i < 0.0)
              throw Exception ("Error converting negative floating-point number to a time");
            hour = std::floor (i / 3600.0); i -= hour * 3600.0;
            if (hour >= 24)
              throw Exception ("Error converting floating-point number to a time: Beyond 24 hours");
            minute = std::floor (i / 60.0); i -= minute * 60;
            second = std::floor (i);
            fraction = i - second;
          }
          Time () : hour (0), minute (0), second (0), fraction (0.0) { }
          operator default_type() const { return (hour*3600.0 + minute*60 + second + fraction); }
          Time operator- (const Time& t) const { return Time (default_type (*this) - default_type (t)); }
          uint32_t hour, minute, second;
          default_type fraction;
          friend std::ostream& operator<< (std::ostream& stream, const Time& item);
      };





      class Element { NOMEMALIGN
        public:
          typedef enum _Type {
            INVALID,
            INT,
            UINT,
            FLOAT,
            DATE,
            TIME,
            STRING,
            SEQ,
            OTHER
          } Type;

          uint16_t group, element, VR;
          uint32_t size;
          uint8_t* data;
          vector<Sequence> parents;
          bool transfer_syntax_supported;

          void set (const std::string& filename, bool force_read = false, bool read_write = false);
          bool read ();

          bool is (uint16_t Group, uint16_t Element) const {
            if (group != Group)
              return false;
            return element == Element;
          }

          std::string tag_name () const {
            if (dict.empty()) init_dict();
            const char* s = dict[tag()];
            return (s ? s : "");
          }

          uint32_t tag () const {
            union __DICOM_group_element_pair__ { uint16_t s[2]; uint32_t i; } val = { {
#if MRTRIX_BYTE_ORDER_BIG_ENDIAN
              group, element
#else
              element, group
#endif
            } };
            return val.i;
          }


          size_t offset (uint8_t* address) const {
            return address - fmap->address();
          }
          bool is_big_endian () const {
            return is_BE;
          }
          bool is_new_sequence () const {
            return VR == VR_SQ || ( group == GROUP_DATA && element == ELEMENT_DATA && size == LENGTH_UNDEFINED );
          }

          Type type () const;
          vector<int32_t> get_int () const;
          vector<uint32_t> get_uint () const;
          vector<default_type> get_float () const;
          Date get_date () const;
          Time get_time () const;
          vector<std::string> get_string () const;

          size_t level () const { return parents.size(); }

          friend std::ostream& operator<< (std::ostream& stream, const Element& item);
          static std::string print_header () {
             return "TYPE  GRP  ELEM VR     SIZE   OFFSET   NAME                                   CONTENTS\n"
                    "----- ---- ---- --  -------  -------   -------------------------------------  ---------------------------------------\n";
          }


        protected:

          std::unique_ptr<File::MMap> fmap;
          void set_explicit_encoding();
          bool read_GR_EL();

          uint8_t* next;
          uint8_t* start;
          bool is_explicit, is_BE, is_transfer_syntax_BE;

          vector<uint8_t*>  end_seq;


          uint16_t get_VR_from_tag_name (const std::string& name) {
            union {
              char t[2];
              uint16_t i;
            } d = { { name[0], name[1] } };
            return ByteOrder::BE (d.i);
          }

          static std::unordered_map<uint32_t, const char*> dict;
          static void init_dict();

          void report_unknown_tag_with_implicit_syntax () const {
            DEBUG (MR::printf ("attempt to read data of unknown value representation "
                  "in DICOM implicit syntax for tag (%02X %02X) - ignored", group, element));
          }
      };



    }
  }
}

#endif

