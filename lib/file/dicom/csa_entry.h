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


    23-09-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * fix handling of ascii floating-point numbers on locales that use a comma as the decimal separator.

    09-09-2009 J-Donald Tournier <d.tournier@brain.org.au>
    * include <cstdio> to allow compilation on Fedora 11

*/

#ifndef __file_dicom_csa_entry_h__
#define __file_dicom_csa_entry_h__

#include "datatype.h"
#include "raw.h"
#include "file/dicom/element.h"

namespace MR {
  namespace File {
    namespace Dicom {

        class CSAEntry {
          public:
            CSAEntry (const uint8_t* start_p, const uint8_t* end_p, bool output_fields = false) :
              start (start_p),
              end (end_p),
              print (output_fields) {
                if (strncmp ("SV10", (const char*) start, 4)) 
                  DEBUG ("WARNING: CSA data is not in SV10 format");

                cnum = 0;
                num = Raw::fetch_LE<uint32_t> (start+8);
                next = start + 16;
              }


            bool parse () {
              if (cnum >= num) 
                return false;
              start = next;
              if (start >= end + 84) 
                return false;
              strncpy (name, (const char*) start, 64);
              Raw::fetch_LE<uint32_t> (start+64); // vm
              strncpy (vr, (const char*) start+68, 4);
              Raw::fetch_LE<uint32_t> (start+72); // syngodt
              nitems = Raw::fetch_LE<uint32_t> (start+76);
              if (print) 
                fprintf (stdout, "    [CSA] %s: ", name);
              next = start + 84;
              if (next + 4 >= end) 
                return false;

              for (uint32_t m = 0; m < nitems; m++) {
                uint32_t length = Raw::fetch_LE<uint32_t> (next);
                size_t size = 16 + 4*((length+3)/4);
                if (next + size > end) 
                  return false;
                if (print) 
                  fprintf (stdout, "%.*s ", length, (const char*) next+16);
                next += size;
              }
              if (print) 
                fprintf (stdout, "\n");

              cnum++;
              return true;
            }

            const char* key () const { return (name); }
            int get_int () const { 
              const uint8_t* p = start + 84;
              for (uint32_t m = 0; m < nitems; m++) {
                uint32_t length = Raw::fetch_LE<uint32_t> (p);
                if (length) 
                  return to<int> (std::string (reinterpret_cast<const char*> (p)+16, 4*((length+3)/4)));
                p += 16 + 4*((length+3)/4);
              }
              return 0;
            }

            float get_float () const { 
              const uint8_t* p = start + 84;
              for (uint32_t m = 0; m < nitems; m++) {
                uint32_t length = Raw::fetch_LE<uint32_t> (p);
                if (length) 
                  return to<float> (std::string (reinterpret_cast<const char*> (p)+16, 4*((length+3)/4)));
                p += 16 + 4*((length+3)/4);
              }
              return NAN;
            }

            void get_float (Eigen::Vector3f& v) const { 
              const uint8_t* p = start + 84;
              for (uint32_t m = 0; m < nitems; m++) {
                uint32_t length = Raw::fetch_LE<uint32_t> (p);
                if (length) 
                  v[m] = to<float> (std::string (reinterpret_cast<const char*> (p)+16, 4*((length+3)/4)));
                p += 16 + 4*((length+3)/4);
              }
            }

            friend std::ostream& operator<< (std::ostream& stream, const CSAEntry& item) {
              stream << "[CSA] " << item.name << ":";
              const uint8_t* next = item.start + 84;

              for (uint32_t m = 0; m < item.nitems; m++) {
                uint32_t length = Raw::fetch_LE<uint32_t> (next);
                size_t size = 16 + 4*((length+3)/4);
                while (length > 0 && !next[16+length-1]) 
                  length--;
                stream << " ";
                stream.write (reinterpret_cast<const char*> (next)+16, length);
                next += size;
              }

              return stream;
            }


          protected:
            const uint8_t* start;
            const uint8_t* next;
            const uint8_t* end;
            bool print;
            char name[65], vr[5];
            uint32_t nitems, num, cnum;

        };





    }
  }
}

#endif


