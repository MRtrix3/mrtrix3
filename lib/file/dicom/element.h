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

#ifndef __file_dicom_element_h__
#define __file_dicom_element_h__

#include <vector>
#include <unordered_map>

#include "memory.h"
#include "raw.h"
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





      class Element { NOMEMALIGN
        public:
          typedef enum _Type {
            INVALID,
            INT,
            UINT,
            FLOAT,
            STRING,
            SEQ,
            OTHER
          } Type;

          uint16_t group, element, VR;
          uint32_t size;
          uint8_t* data;
          vector<Sequence> parents;

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

          Type type () const;
          vector<int32_t> get_int () const;
          vector<uint32_t> get_uint () const;
          vector<double> get_float () const;
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

