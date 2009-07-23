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


    02-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * extra sanity check to make sure that each element fits within the file.

    31-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * only attempt to read a "truncated format" DICOM file if the extension is ".dcm"

    18-12-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * printout of DICOM group & element is now in hexadecimal

    17-03-2009 J-Donald Tournier <d.tournier@brain.org.au>
    * modify to allow use of either TR1 unordered map or SGI hash_map for the DICOM dictionary
    
*/

#include "file/path.h"
#include "file/dicom/element.h"
#include "get_set.h"

namespace MR {
  namespace File {
    namespace Dicom {

      void Element::set (const std::string& filename)
      {
        group = element = VR = 0;
        size = 0;
        start = data = next = NULL;
        is_BE = previous_BO_was_BE = false;
        end_seq.clear();
        item_number.clear();

        fmap.init (filename);
        if (fmap.size() < 256) throw Exception ("\"" + fmap.name() + "\" is too small to be a valid DICOM file", 3);
        fmap.map();

        next = (uint8_t*) fmap.address();

        if (memcmp (next + 128, "DICM", 4)) {
          is_explicit = false;
          debug ("DICOM magic number not found in file \"" + fmap.name() + "\" - trying truncated format");
          if (!Path::has_suffix (fmap.name(), ".dcm")) 
            throw Exception ("file \"" + fmap.name() + "\" does not have the DICOM magic number or the .dcm extension - assuming not DICOM");
        }
        else next += 132;

        try { set_explicit_encoding(); }
        catch (Exception) {
          fmap.unmap();
          throw Exception ("\"" + fmap.name() + "\" is not a valid DICOM file", 3);
        }
      }






      void Element::set_explicit_encoding ()
      {
        if (read_GR_EL()) throw Exception ("\"" + fmap.name() + "\" is too small to be DICOM", 3);

        is_explicit = true;
        next = start;
        VR = ByteOrder::BE (*((uint16_t*) (start+4)));

        if ((VR == VR_OB) | (VR == VR_OW) | (VR == VR_OF) | (VR == VR_SQ) |
            (VR == VR_UN) | (VR == VR_AE) | (VR == VR_AS) | (VR == VR_AT) |
            (VR == VR_CS) | (VR == VR_DA) | (VR == VR_DS) | (VR == VR_DT) |
            (VR == VR_FD) | (VR == VR_FL) | (VR == VR_IS) | (VR == VR_LO) |
            (VR == VR_LT) | (VR == VR_PN) | (VR == VR_SH) | (VR == VR_SL) |
            (VR == VR_SS) | (VR == VR_ST) | (VR == VR_TM) | (VR == VR_UI) |
            (VR == VR_UL) | (VR == VR_US) | (VR == VR_UT)) return;

        debug ("using implicit DICOM encoding");
        is_explicit = false;
      }






      bool Element::read_GR_EL ()
      {
        group = element = VR = 0;
        size = 0;
        start = next;
        data = next = NULL;

        if (start < (uint8_t*) fmap.address()) throw Exception ("invalid DICOM element", 3);
        if (start + 8 > (uint8_t*) fmap.address() + fmap.size()) return (true);

        is_BE = previous_BO_was_BE;

        group = get<uint16_t> (start, is_BE);

        if (group == GROUP_BYTE_ORDER_SWAPPED) {
          if (!is_BE) 
            throw Exception ("invalid DICOM group ID " + str (group) + " in file \"" + fmap.name() + "\"", 3);

          is_BE = false;
          group = GROUP_BYTE_ORDER;
        }
        element = get<uint16_t> (start+2, is_BE);

        return (false);
      }






      bool Element::read ()
      {
        if (read_GR_EL()) return (false);

        data = start + 8;
        if ((is_explicit && group != GROUP_SEQUENCE) || group == GROUP_BYTE_ORDER) {
          // explicit encoding:
          VR = ByteOrder::BE (*((uint16_t*) (start+4)));
          if (VR == VR_OB || VR == VR_OW || VR == VR_OF || VR == VR_SQ || VR == VR_UN || VR == VR_UT) {
            size = get<uint32_t> (start+8, is_BE);
            data += 4;
          }
          else size = get<uint16_t> (start+6, is_BE);
        }
        else {
          // implicit encoding:
          std::string name = tag_name();
          if (!name.size()) {
            if (group%2 == 0) 
              debug ("WARNING: unknown DICOM tag (" + str (group) + ", " + str (element) 
                  + ") with implicit encoding in file \"" + fmap.name() + "\"");
            VR = VR_UN;
          }
          else {
            char t[] = { name[0], name[1] };
            VR = ByteOrder::BE (*((uint16_t*) t));
          }
          size = get<uint32_t> (start+4, is_BE);
        }


        next = data;
        if (size == LENGTH_UNDEFINED) {
          if (VR != VR_SQ && !(group == GROUP_SEQUENCE && element == ELEMENT_SEQUENCE_ITEM)) 
            throw Exception ("undefined length used for DICOM tag " + ( tag_name().size() ? tag_name().substr (2) : "" ) 
                + " (" + str (group) + ", " + str (element) 
                + ") in file \"" + fmap.name() + "\"", 3);
        }
        else if (next+size > (uint8_t*) fmap.address() + fmap.size()) throw Exception ("file \"" + fmap.name() + "\" is too small to contain DICOM elements specified", 3);
        else if (size%2) throw Exception ("odd length (" + str (size) + ") used for DICOM tag " + ( tag_name().size() ? tag_name().substr (2) : "" ) 
              + " (" + str (group) + ", " + str (element) + ") in file \"" + fmap.name() + "", 3);
        else if (VR != VR_SQ && ( group != GROUP_SEQUENCE || element != ELEMENT_SEQUENCE_ITEM ) ) next += size;



        if (VR == VR_SQ) {
          if (size == LENGTH_UNDEFINED) end_seq.push_back (NULL); 
          else end_seq.push_back (data + size);
          item_number.push_back (0);
        }

        if (end_seq.size()) {
          if ((end_seq.back() && data > end_seq.back()) || (group == GROUP_SEQUENCE && element == ELEMENT_SEQUENCE_DELIMITATION_ITEM)) {
            end_seq.pop_back();
            item_number.pop_back();
          }
        }

        if (group == GROUP_SEQUENCE && element == ELEMENT_SEQUENCE_ITEM) item_number.back()++;



        switch (group) {
          case GROUP_BYTE_ORDER:
            switch (element) {
              case ELEMENT_TRANSFER_SYNTAX_UID:
                if (strncmp ((const char*) data, "1.2.840.10008.1.2.1", size) == 0) {
                  is_BE = false; // explicit VR Little Endian
                  is_explicit = true;
                }
                else if (strncmp ((const char*) data, "1.2.840.10008.1.2.2", size) == 0) {
                  is_BE = true; // Explicit VR Big Endian
                  is_explicit = true;
                }
                else if (strncmp ((const char*) data, "1.2.840.10008.1.2", size) == 0) {
                  is_BE = false; // Implicit VR Little Endian
                  is_explicit = false;
                }
                else if (strncmp ((const char*) data, "1.2.840.10008.1.2.1.99", size) == 0) {
                  throw Exception ("DICOM deflated explicit VR little endian transfer syntax not supported");
                }
                else error ("unknown DICOM transfer syntax: \"" + std::string ((const char*) data, size) 
                    + "\" in file \"" + fmap.name() + "\" - ignored");
                break;
            }

            break;
        }

        return (true);
      }












      ElementType Element::type () const
      {
        if (!VR) return (INVALID);
        if (VR == VR_FD || VR == VR_FL) return (FLOAT);
        if (VR == VR_SL || VR == VR_SS) return (INT);
        if (VR == VR_UL || VR == VR_US) return (UINT);
        if (VR == VR_SQ) return (SEQ);
        if (VR == VR_AE || VR == VR_AS || VR == VR_CS || VR == VR_DA ||
            VR == VR_DS || VR == VR_DT || VR == VR_IS || VR == VR_LO ||
            VR == VR_LT || VR == VR_PN || VR == VR_SH || VR == VR_ST ||
            VR == VR_TM || VR == VR_UI || VR == VR_UT) return (STRING);
        return (OTHER);
      }



      std::vector<int32_t> Element::get_int () const
      {
        std::vector<int32_t> V;
        if (VR == VR_SL) 
          for (const uint8_t* p = data; p < data + size; p += sizeof (int32_t))
            V.push_back (get<int32_t> (p, is_BE));
        else if (VR == VR_SS)
          for (const uint8_t* p = data; p < data + size; p += sizeof (int16_t)) 
            V.push_back (get<int16_t> (p, is_BE));
        else if (VR == VR_IS) {
          std::vector<std::string> strings (get_string ());
          V.resize (strings.size());
          for (uint n = 0; n < V.size(); n++) V[n] = to<int32_t> (strings[n]);
        }
        return (V);
      }




      std::vector<uint32_t> Element::get_uint () const
      {
        std::vector<uint32_t> V;
        if (VR == VR_UL) 
          for (const uint8_t* p = data; p < data + size; p += sizeof (uint32_t))
            V.push_back (get<uint32_t> (p, is_BE));
        else if (VR == VR_US)
          for (const uint8_t* p = data; p < data + size; p += sizeof (uint16_t)) 
            V.push_back (get<uint16_t> (p, is_BE));
        else if (VR == VR_IS) {
          std::vector<std::string> strings (split (std::string ((const char*) data, size), "\\", false));
          V.resize (strings.size());
          for (uint n = 0; n < V.size(); n++) V[n] = to<uint32_t> (strings[n]);
        }
        return (V);
      }



      std::vector<double> Element::get_float () const
      {
        std::vector<double> V;
        if (VR == VR_FD) 
          for (const uint8_t* p = data; p < data + size; p += sizeof (float64))
            V.push_back (get<float64> (p, is_BE));
        else if (VR == VR_FL)
          for (const uint8_t* p = data; p < data + size; p += sizeof (float32)) 
            V.push_back (get<float32> (p, is_BE));
        else if (VR == VR_DS) {
          std::vector<std::string> strings (split (std::string ((const char*) data, size), "\\", false));
          V.resize (strings.size());
          for (uint n = 0; n < V.size(); n++) V[n] = to<double> (strings[n]);
        }
        return (V);
      }



      

      std::vector<std::string> Element::get_string () const
      { 
        std::vector<std::string> strings (split (std::string ((const char*) data, size), "\\", false)); 
        for (std::vector<std::string>::iterator i = strings.begin(); i != strings.end(); ++i) {
          *i = strip (*i);
          replace (*i, '^', ' ');
        }
        return (strings);
      }



      namespace {
        template <class T> inline void print_vec (const std::vector<T>& V)
        { for (uint n = 0; n < V.size(); n++) fprintf (stdout, "%s ", str (V[n]).c_str()); }
      }






      void Element::print() const
      {
        std::string name = tag_name();
        fprintf (stdout, "  [DCM] %*s : ", int(2*end_seq.size()), ( name.size() ? name.substr(2).c_str() : "unknown" ));
        switch (type()) {
          case INT: print_vec (get_int()); break;
          case UINT: print_vec (get_int()); break;
          case FLOAT: print_vec (get_int()); break;
          case STRING: 
            if (group == GROUP_DATA && element == ELEMENT_DATA) fprintf (stdout, "(data)");
            else print_vec (get_int());
            break;
          case SEQ: fprintf (stdout, "(sequence)"); break;
          default: fprintf (stdout, "unknown data type");
        }
        if (group%2) fprintf (stdout, " [ PRIVATE ]\n");
        else fprintf (stdout, "\n");
      }







      std::ostream& operator<< (std::ostream& stream, const Element& item)
      {
        const std::string& name (item.tag_name());

        stream << "[DCM] ";
        for (uint i = 0; i < item.end_seq.size(); i++) stream << "  ";
        stream << printf ("%02X %02X ", item.group, item.element)  
            + ((const char*) &item.VR)[1] + ((const char*) &item.VR)[0] + " " 
            + str ( item.size == LENGTH_UNDEFINED ? 0 : item.size ) + " " 
            + str (item.offset (item.start)) + " " + ( name.size() ? name.substr (2) : "unknown" ) + " : ";


        switch (item.type()) {
          case INT: stream << item.get_int(); break;
          case UINT: stream << item.get_uint(); break;
          case FLOAT: stream << item.get_float(); break;
          case STRING:
            if (item.group == GROUP_DATA && item.element == ELEMENT_DATA) stream << "(data)";
            else stream << item.get_string(); 
            break;
          case SEQ:
            stream << "(sequence)";
            break;
          default:
            stream << "unknown data type";
        }
        if (item.group%2) stream << " [ PRIVATE ]";

        if (item.item_number.size()) {
          stream << " [ ";
          for (uint n = 0; n < item.item_number.size(); n++) stream << item.item_number[n] << " ";
          stream << "] ";
        }

        return (stream);
      }


    }
  }
}

