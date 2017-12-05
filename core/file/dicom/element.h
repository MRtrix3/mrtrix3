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
          vector<double> get_float () const;
          vector<std::string> get_string () const;

          int32_t     get_int (size_t idx, int32_t default_value = 0)                    const { auto v (get_int());    return check_get (idx, v.size()) ? v[idx] : default_value; }
          uint32_t    get_uint (size_t idx, uint32_t default_value = 0)                  const { auto v (get_uint());   return check_get (idx, v.size()) ? v[idx] : default_value; }
          double      get_float (size_t idx, double default_value = 0.0)                 const { auto v (get_float());  return check_get (idx, v.size()) ? v[idx] : default_value; }
          std::string get_string (size_t idx, std::string default_value = std::string()) const { auto v (get_string()); return check_get (idx, v.size()) ? v[idx] : default_value; }

          size_t level () const { return parents.size(); }

          friend std::ostream& operator<< (std::ostream& stream, const Element& item);
          static std::string print_header () {
             return "TYPE  GRP  ELEM VR     SIZE   OFFSET   NAME                                   CONTENTS\n"
                    "----- ---- ---- --  -------  -------   -------------------------------------  ---------------------------------------\n";
          }


          template <typename VectorType> 
            FORCE_INLINE void check_size (const VectorType v, size_t min_size = 1) { 
              if (v.size() < min_size)
                error_in_check_size (min_size, v.size());
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

          bool check_get (size_t idx, size_t size) const { if (idx >= size) { error_in_get (idx); return false; } return true; }
          void error_in_get (size_t idx) const; 
          void error_in_check_size (size_t min_size, size_t actual_size) const; 
          void report_unknown_tag_with_implicit_syntax () const;
      };



    }
  }
}

#endif

