/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.


    17-03-2009 J-Donald Tournier <d.tournier@brain.org.au>
    * modify to allow use of either TR1 unordered map or SGI hash_map for the DICOM dictionary
    
*/

#ifndef __file_dicom_element_h__
#define __file_dicom_element_h__

#include "hash_map.h"
#include "file/mmap.h"
#include "file/dicom/definitions.h"

namespace MR {
  namespace File {
    namespace Dicom {

      typedef enum _ElementType {
        INVALID,
        INT,
        UINT,
        FLOAT,
        STRING,
        SEQ,
        OTHER
      } ElementType;





      class Element {
        protected:
          static UnorderedMap<uint32_t, const char*>::Type dict;
          static void           init_dict();

          File::MMap            fmap;
          void                  set_explicit_encoding();
          bool                  read_GR_EL();

          uint8_t*               next;
          uint8_t*               start;
          bool                  is_explicit;
          bool                  is_BE;
          bool                  previous_BO_was_BE;

          std::vector<uint8_t*>  end_seq;

        public:

          uint16_t               group, element, VR;
          uint32_t               size;
          uint8_t*               data;

          std::vector<uint>    item_number;

          void                  set (const std::string& filename);
          bool                  read ();

          bool                  is (uint16_t group, uint16_t element) const;

          std::string                tag_name () const;
          uint32_t               tag () const;
          ElementType           type () const;
          bool                  is_big_endian () const;
          std::vector<int32_t>   get_int () const;
          std::vector<uint32_t>  get_uint () const;
          std::vector<double>   get_float () const;
          std::vector<std::string>   get_string () const;
          uint                 offset (uint8_t* address) const;

          void                  print () const;

          friend std::ostream& operator<< (std::ostream& stream, const Element& item);
      };



















      inline bool Element::is (uint16_t Group, uint16_t Element) const
      {
        if (group != Group) return (false);
        return (element == Element);
      }





      inline bool Element::is_big_endian () const { return (is_BE); }




      inline uint32_t Element::tag () const
      {
        union __DICOM_group_element_pair__ { uint16_t s[2]; uint32_t i; } val = { {
#ifdef BYTE_ORDER_BIG_ENDIAN
          group, element
#else
            element, group
#endif 
        } };
        return (val.i);
      }





      inline std::string Element::tag_name() const
      {
        if (dict.empty()) init_dict();
        const char* s = dict[tag()];
        return (s ? s : "");
      }





      inline uint Element::offset (uint8_t* address) const { return (address - (uint8_t*) fmap.address()); }






    }
  }
}

#endif

