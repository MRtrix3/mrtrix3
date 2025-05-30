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

#ifndef __file_dicom_csa_entry_h__
#define __file_dicom_csa_entry_h__

#include "datatype.h"
#include "raw.h"
#include "types.h"
#include "file/dicom/element.h"

namespace MR {
  namespace File {
    namespace Dicom {

        class CSAEntry { NOMEMALIGN
          public:
            CSAEntry (const uint8_t* start_p, const uint8_t* end_p, bool output_fields = false) :
                start (start_p),
                end (end_p),
                print (output_fields),
                cnum (0)
            {
              if (strncmp ("SV10", (const char*) start, 4)) {
                DEBUG ("Siemens CSA entry does not start with \"SV10\"; ignoring");
                num = 0;
                next = end;
              } else {
                const uint8_t* const unused1 = start+4;
                if (unused1[0] != 0x04U || unused1[1] != 0x03U || unused1[2] != 0x02U || unused1[3] != 0x01U)
                  DEBUG ("WARNING: CSA2 \'unused1\' int8 field contains unexpected data");
                num = Raw::fetch_LE<uint32_t> (start+8);
                const uint32_t unused2 = Raw::fetch_LE<uint32_t> (start+12);
                if (unused2 != 77)
                  DEBUG ("CSA2 \'unused2\' integer field contains " + str(unused2) + "; expected 77");
                next = start + 16;
              }
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
              const int32_t xx = Raw::fetch_LE<int32_t> (start+80);
              if (!(xx == 77 || xx == 205))
                DEBUG ("CSA tag \'xx\' integer field contains " + str(xx) + "; expected 77 or 205");
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

            const char* key () const { return name; }

            uint32_t num_items() const { return nitems; }
            uint32_t size() const { return num; }

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

            default_type get_float () const {
              const uint8_t* p = start + 84;
              for (uint32_t m = 0; m < nitems; m++) {
                uint32_t length = Raw::fetch_LE<uint32_t> (p);
                if (length)
                  return to<default_type> (std::string (reinterpret_cast<const char*> (p)+16, 4*((length+3)/4)));
                p += 16 + 4*((length+3)/4);
              }
              return NaN;
            }

            template <typename Container>
              void get_float (Container& v) const {
                const uint8_t* p = start + 84;
                if (nitems < v.size())
                  DEBUG ("CSA entry contains fewer items than expected - trailing entries will be set to NaN");
                for (uint32_t m = 0; m < std::min<size_t> (nitems, v.size()); m++) {
                  uint32_t length = Raw::fetch_LE<uint32_t> (p);
                  v[m] = length ? to<default_type> (std::string (reinterpret_cast<const char*> (p)+16, 4*((length+3)/4))) : NaN;
                  p += 16 + 4*((length+3)/4);
                }
                for (uint32_t m = nitems; m < v.size(); ++m)
                  v[m] = NaN;
              }

            vector<std::string> get_string () const {
              vector<std::string> result;
              const uint8_t* p = start + 84;
              for (uint32_t m = 0; m < nitems; m++) {
                const uint32_t length = Raw::fetch_LE<uint32_t> (p);
                std::string s (reinterpret_cast<const char*> (p)+16, length);
                result.push_back (std::move (s));
                p += 16 + 4*((length+3)/4);
              }
              return result;
            }

            friend std::ostream& operator<< (std::ostream& stream, const CSAEntry& item) {
              stream << "[CSA] " << item.name << " (" + str(item.nitems) + " items):";
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
              stream << "\n";

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


