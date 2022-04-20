/* Copyright (c) 2008-2022 the MRtrix3 contributors.
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

#include "file/path.h"
#include "file/dicom/element.h"
#include "debug.h"

namespace MR {
  namespace File {
    namespace Dicom {



      std::ostream& operator<< (std::ostream& stream, const Date& item)
      {
        stream << item.year << "/"
               << std::setfill('0') << std::setw(2) << item.month << "/"
               << std::setfill('0') << std::setw(2) << item.day;
        return stream;
      }





      std::ostream& operator<< (std::ostream& stream, const Time& item)
      {
        stream << std::setfill('0') << std::setw(2) << item.hour << ":"
               << std::setfill('0') << std::setw(2) << item.minute << ":"
               << std::setfill('0') << std::setw(2) << item.second;
        if (item.fraction)
          stream << str(item.fraction, 6).substr(1);
        return stream;
      }





      const char* Element::type_as_str[] = { "invalid",
                                             "integer",
                                             "unsigned integer",
                                             "floating-point",
                                             "date",
                                             "time",
                                             "string",
                                             "sequence",
                                             "other",
                                             nullptr };






      void Element::set (const std::string& filename, bool force_read, bool read_write)
      {
        group = element = VR = 0;
        size = 0;
        start = data = next = NULL;
        is_BE = is_transfer_syntax_BE = false;
        transfer_syntax_supported = true;
        parents.clear();

        fmap.reset (new File::MMap (filename, read_write));

        if (fmap->size() < 256)
          throw Exception ("\"" + fmap->name() + "\" is too small to be a valid DICOM file");

        next = fmap->address();

        if (memcmp (next + 128, "DICM", 4)) {
          is_explicit = false;
          DEBUG ("DICOM magic number not found in file \"" + fmap->name() + "\" - trying truncated format");
          if (!force_read)
            if (!Path::has_suffix (fmap->name(), ".dcm"))
              throw Exception ("file \"" + fmap->name() + "\" does not have the DICOM magic number or the .dcm extension - assuming not DICOM");
        }
        else next += 132;

        try { set_explicit_encoding(); }
        catch (Exception) {
          throw Exception ("\"" + fmap->name() + "\" is not a valid DICOM file");
          fmap.reset();
        }
      }






      void Element::set_explicit_encoding ()
      {
        assert (fmap);
        if (read_GR_EL())
          throw Exception ("\"" + fmap->name() + "\" is too small to be DICOM");

        is_explicit = true;
        next = start;
        VR = ByteOrder::BE (*reinterpret_cast<uint16_t*> (start+4));

        if ((VR == VR_OB) | (VR == VR_OW) | (VR == VR_OF) | (VR == VR_SQ) |
            (VR == VR_UN) | (VR == VR_AE) | (VR == VR_AS) | (VR == VR_AT) |
            (VR == VR_CS) | (VR == VR_DA) | (VR == VR_DS) | (VR == VR_DT) |
            (VR == VR_FD) | (VR == VR_FL) | (VR == VR_IS) | (VR == VR_LO) |
            (VR == VR_LT) | (VR == VR_PN) | (VR == VR_SH) | (VR == VR_SL) |
            (VR == VR_SS) | (VR == VR_ST) | (VR == VR_TM) | (VR == VR_UI) |
            (VR == VR_UL) | (VR == VR_US) | (VR == VR_UT)) return;

        DEBUG ("using implicit DICOM encoding");
        is_explicit = false;
      }






      bool Element::read_GR_EL ()
      {
        group = element = VR = 0;
        size = 0;
        start = next;
        data = next = NULL;

        if (start < fmap->address())
          throw Exception ("invalid DICOM element");

        if (start + 8 > fmap->address() + fmap->size())
          return true;

        is_BE = is_transfer_syntax_BE;

        group = Raw::fetch_<uint16_t> (start, is_BE);

        if (group == GROUP_BYTE_ORDER_SWAPPED) {
          if (!is_BE)
            throw Exception ("invalid DICOM group ID " + str (group) + " in file \"" + fmap->name() + "\"");

          is_BE = false;
          group = GROUP_BYTE_ORDER;
        }
        element = Raw::fetch_<uint16_t> (start+2, is_BE);

        return false;
      }






      bool Element::read ()
      {
        if (read_GR_EL())
          return false;

        data = start + 8;
        if ((is_explicit && group != GROUP_SEQUENCE) || group == GROUP_BYTE_ORDER) {

          // explicit encoding:
          VR = ByteOrder::BE (*reinterpret_cast<uint16_t*> (start+4));
          if (VR == VR_OB || VR == VR_OW || VR == VR_OF || VR == VR_SQ || VR == VR_UN || VR == VR_UT) {
            size = Raw::fetch_<uint32_t> (start+8, is_BE);
            data += 4;
          }
          else size = Raw::fetch_<uint16_t> (start+6, is_BE);

          // try figuring out VR from dictionary if vendors haven't bothered
          // filling it in...
          if (VR == VR_UN) {
            std::string name = tag_name();
            if (name.size())
              VR = get_VR_from_tag_name (name);
          }
        }
        else {

          // implicit encoding:
          std::string name = tag_name();
          if (!name.size()) {
            DEBUG (printf ("WARNING: unknown DICOM tag (%04X %04X) "
                  "with implicit encoding in file \"", group, element)
                + fmap->name() + "\"");
            VR = VR_UN;
          }
          else
            VR = get_VR_from_tag_name (name);
          size = Raw::fetch_<uint32_t> (start+4, is_BE);
        }

        next = data;

        if (size == LENGTH_UNDEFINED) {
          if (VR != VR_SQ && !(group == GROUP_SEQUENCE && element == ELEMENT_SEQUENCE_ITEM))
            INFO ("undefined length used for DICOM tag " + ( tag_name().size() ? tag_name().substr (2) : "" )
                + MR::printf ("(%04X, %04X) in file \"", group, element) + fmap->name() + "\"");
        }
        else if (next+size > fmap->address() + fmap->size())
          throw Exception ("file \"" + fmap->name() + "\" is too small to contain DICOM elements specified");
        else {
          if (size%2)
            DEBUG ("WARNING: odd length (" + str (size) + ") used for DICOM tag " + ( tag_name().size() ? tag_name().substr (2) : "" )
                + " (" + str (group) + ", " + str (element) + ") in file \"" + fmap->name() + "");
          if (VR != VR_SQ) {
            if (group == GROUP_SEQUENCE && element == ELEMENT_SEQUENCE_ITEM) {
              if (parents.size() && parents.back().group == GROUP_DATA && parents.back().element == ELEMENT_DATA)
                next += size;
            }
            else
              next += size;
          }
        }



        if (parents.size())
          if ((parents.back().end && data > parents.back().end) ||
              (group == GROUP_SEQUENCE && element == ELEMENT_SEQUENCE_DELIMITATION_ITEM))
            parents.pop_back();

        if (is_new_sequence()) {
          if (size == LENGTH_UNDEFINED)
            parents.push_back (Sequence (group, element, nullptr));
          else
            parents.push_back (Sequence (group, element, data + size));
        }




        switch (group) {
          case GROUP_BYTE_ORDER:
            switch (element) {
              case ELEMENT_TRANSFER_SYNTAX_UID:
                if (strncmp (reinterpret_cast<const char*> (data), "1.2.840.10008.1.2.1", size) == 0) {
                  is_BE = is_transfer_syntax_BE = false; // explicit VR Little Endian
                  is_explicit = true;
                }
                else if (strncmp (reinterpret_cast<const char*> (data), "1.2.840.10008.1.2.2", size) == 0) {
                  is_BE = is_transfer_syntax_BE = true; // Explicit VR Big Endian
                  is_explicit = true;
                }
                else if (strncmp (reinterpret_cast<const char*> (data), "1.2.840.10008.1.2", size) == 0) {
                  is_BE = is_transfer_syntax_BE = false; // Implicit VR Little Endian
                  is_explicit = false;
                }
                else if (strncmp (reinterpret_cast<const char*> (data), "1.2.840.10008.1.2.1.99", size) == 0) {
                  throw Exception ("DICOM deflated explicit VR little endian transfer syntax not supported");
                }
                else {
                  transfer_syntax_supported = false;
                  INFO ("unsupported DICOM transfer syntax: \"" + std::string (reinterpret_cast<const char*> (data), size)
                    + "\" in file \"" + fmap->name() + "\"");
                }
                break;
            }

            break;
        }

        return true;
      }





      bool Element::ignore_when_parsing () const
      {
        for (const auto& seq : parents) {
          // ignore anything within IconImageSequence:
          if (seq.is (0x0088U, 0x0200U))
            return true;
          // allow Philips PrivatePerFrameSq:
          if (seq.is (0x2005U, 0x140FU))
            continue;
          // ignore anything within sequences with unknown (private) group:
          if (seq.group & 1U)
            return true;
        }

        return false;
      }





      bool Element::is_in_series_ref_sequence () const
      {
        // required to group together series exported using
        // Siemens XA10A in Interoperability mode
        for (const auto& seq : parents)
          if (seq.is (0x0008U, 0x1250U))
            return true;
        return false;
      }




      Element::Type Element::type () const
      {
        if (!VR) return INVALID;
        if (VR == VR_FD || VR == VR_FL) return FLOAT;
        if (VR == VR_SL || VR == VR_SS) return INT;
        if (VR == VR_UL || VR == VR_US) return UINT;
        if (VR == VR_SQ) return SEQ;
        if (VR == VR_DA) return DATE;
        if (VR == VR_TM) return TIME;
        if (VR == VR_DT) return DATETIME;
        if (VR == VR_AE || VR == VR_AS || VR == VR_CS ||
            VR == VR_DS || VR == VR_IS || VR == VR_LO ||
            VR == VR_LT || VR == VR_PN || VR == VR_SH || VR == VR_ST ||
            VR == VR_UI || VR == VR_UT || VR == VR_AT) return STRING;
        return OTHER;
      }



      vector<int32_t> Element::get_int () const
      {
        vector<int32_t> V;
        if (VR == VR_SL)
          for (const uint8_t* p = data; p < data + size; p += sizeof (int32_t))
            V.push_back (Raw::fetch_<int32_t> (p, is_BE));
        else if (VR == VR_SS)
          for (const uint8_t* p = data; p < data + size; p += sizeof (int16_t))
            V.push_back (Raw::fetch_<int16_t> (p, is_BE));
        else if (VR == VR_IS) {
          auto strings = split (std::string (reinterpret_cast<const char*> (data), size), "\\", false);
          V.resize (strings.size());
          for (size_t n = 0; n < V.size(); n++)
            V[n] = to<int32_t> (strings[n]);
        }
        else
          report_unknown_tag_with_implicit_syntax();

        return V;
      }




      vector<uint32_t> Element::get_uint () const
      {
        vector<uint32_t> V;
        if (VR == VR_UL)
          for (const uint8_t* p = data; p < data + size; p += sizeof (uint32_t))
            V.push_back (Raw::fetch_<uint32_t> (p, is_BE));
        else if (VR == VR_US)
          for (const uint8_t* p = data; p < data + size; p += sizeof (uint16_t))
            V.push_back (Raw::fetch_<uint16_t> (p, is_BE));
        else if (VR == VR_IS) {
          auto strings = split (std::string (reinterpret_cast<const char*> (data), size), "\\", false);
          V.resize (strings.size());
          for (size_t n = 0; n < V.size(); n++)
            V[n] = to<uint32_t> (strings[n]);
        }
        else
          report_unknown_tag_with_implicit_syntax();
        return V;
      }



      vector<default_type> Element::get_float () const
      {
        vector<default_type> V;
        if (VR == VR_FD)
          for (const uint8_t* p = data; p < data + size; p += sizeof (float64))
            V.push_back (Raw::fetch_<float64> (p, is_BE));
        else if (VR == VR_FL)
          for (const uint8_t* p = data; p < data + size; p += sizeof (float32))
            V.push_back (Raw::fetch_<float32> (p, is_BE));
        else if (VR == VR_DS || VR == VR_IS) {
          auto strings = split (std::string (reinterpret_cast<const char*> (data), size), "\\", false);
          V.resize (strings.size());
          for (size_t n = 0; n < V.size(); n++)
            V[n] = to<default_type> (strings[n]);
        }
        else
          report_unknown_tag_with_implicit_syntax();
        return V;
      }




      Date Element::get_date () const
      {
        assert (type() == DATE);
        return Date (std::string (reinterpret_cast<const char*> (data), size));
      }




      Time Element::get_time () const
      {
        assert (type() == TIME);
        return Time (std::string (reinterpret_cast<const char*> (data), size));
      }




      std::pair<Date,Time> Element::get_datetime () const
      {
        assert (type() == DATETIME);
        if (size < 21)
          throw Exception ("malformed DateTime entry");
        return {
          Date (std::string (reinterpret_cast<const char*> (data), 8)),
          Time (std::string (reinterpret_cast<const char*> (data+8), 13))
        };
      }





      vector<std::string> Element::get_string () const
      {
        if (VR == VR_AT)
          return { printf ("%04X %04X", Raw::fetch_<uint16_t> (data, is_BE), Raw::fetch_<uint16_t> (data+2, is_BE)) };

        auto strings = split (std::string (reinterpret_cast<const char*> (data), size), "\\", false);
        for (auto& entry: strings)
          entry = strip (entry);
        return strings;
      }




      std::string Element::as_string () const
      {
        std::ostringstream out;
        try {
          switch (type()) {
            case Element::INT:
              for (const auto& x : get_int())
                out << x << " ";
              return out.str();
            case Element::UINT:
              for (const auto& x : get_uint())
                out << x << " ";
              return out.str();
            case Element::FLOAT:
              for (const auto& x : get_float())
                out << x << " ";
              return out.str();
            case Element::DATE:
              return str(get_date());
            case Element::TIME:
              return str(get_time());
            case Element::STRING:
              if (group == GROUP_DATA && element == ELEMENT_DATA) {
                return "(data)";
              }
              else {
                for (const auto& x : get_string())
                  out << x << " ";
                return out.str();
              }
            case Element::SEQ:
              return "";
            default:
              if (group != GROUP_SEQUENCE || element != ELEMENT_SEQUENCE_ITEM)
                return "unknown data type";
          }
        }
        catch (Exception& e) {
          DEBUG ("Error converting data at offset " + str(offset(start)) + " to " + type_as_str[type()] + " type: ");
          for (auto& s : e.description)
            DEBUG (s);
          return "invalid entry";
        }
        return "";
      }






      namespace {
        template <class T>
          inline void print_vec (const vector<T>& V)
          {
            for (const auto& entry: V)
              fprintf (stdout, "%s ", str(entry).c_str());
          }
      }


      void Element::error_in_get (size_t idx) const
      {
        const std::string& name (tag_name());
        DEBUG ("value not found for DICOM tag " + printf("%04X %04X ", group, element) + ( name.size() ? name.substr(2) : "unknown" ) + " (at index " + str(idx) + ")");
      }

      void Element::error_in_check_size (size_t min_size, size_t actual_size) const
      {
        const std::string& name (tag_name());
        throw Exception ("not enough items in for DICOM tag " + printf("%04X %04X ", group, element) + ( name.size() ? name.substr(2) : "unknown" ) + " (expected " + str(min_size) + ", got " + str(actual_size) + ")");
      }

      void Element::report_unknown_tag_with_implicit_syntax () const
      {
        DEBUG (MR::printf ("attempt to read data of unknown value representation "
              "in DICOM implicit syntax for tag (%04X %04X) - ignored", group, element));
      }








      std::ostream& operator<< (std::ostream& stream, const Element& item)
      {
        //return "TYPE  GROUP ELEMENT VR  SIZE  OFFSET  NAME                               CONTENTS";

        const std::string& name (item.tag_name());
        stream << printf ("[DCM] %04X %04X %c%c % 8u % 8llu ", item.group, item.element,
            reinterpret_cast<const char*> (&item.VR)[1], reinterpret_cast<const char*> (&item.VR)[0],
            ( item.size == LENGTH_UNDEFINED ? uint32_t(0) : item.size ), item.offset (item.start));

        std::string tmp;
        size_t indent = item.level() - ( item.VR == VR_SQ ? 1 : 0 );
        for (size_t i = 0; i < indent; i++)
          tmp += "  ";
        if (item.is_new_sequence())
          tmp += "> ";
        else if (item.group == GROUP_SEQUENCE && item.element == ELEMENT_SEQUENCE_ITEM)
          tmp += "- ";
        else
          tmp += "  ";
        tmp += ( name.size() ? name.substr(2) : "unknown" );
        tmp.resize (40, ' ');
        stream << tmp << " " << item.as_string() << "\n";

        return stream;
      }



    }
  }
}

