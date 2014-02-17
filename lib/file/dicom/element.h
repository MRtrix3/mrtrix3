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

