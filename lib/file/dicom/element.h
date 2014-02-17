#ifndef __file_dicom_element_h__
#define __file_dicom_element_h__

#include <vector>

#include "ptr.h"
#include "hash_map.h"
#include "file/mmap.h"
#include "file/dicom/definitions.h"

namespace MR {
  namespace File {
    namespace Dicom {


      class Sequence {
        public:
          Sequence (uint16_t group, uint16_t element, uint8_t* end) : group (group), element (element), end (end) { }
          uint16_t group, element;
          uint8_t* end;
      };





      class Element {
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
          std::vector<Sequence> parents;

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
          std::vector<int32_t> get_int () const;
          std::vector<uint32_t> get_uint () const;
          std::vector<double> get_float () const;
          std::vector<std::string> get_string () const;

          size_t level () const { return parents.size(); }

          friend std::ostream& operator<< (std::ostream& stream, const Element& item);


        protected:

          Ptr<File::MMap> fmap;
          void set_explicit_encoding();
          bool read_GR_EL();

          uint8_t* next;
          uint8_t* start;
          bool is_explicit, is_BE, is_transfer_syntax_BE;

          std::vector<uint8_t*>  end_seq;

          static UnorderedMap<uint32_t, const char*>::Type dict;
          static void init_dict();
      };








































    }
  }
}

#endif

