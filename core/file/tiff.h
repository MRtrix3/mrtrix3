/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#ifndef __file_tiff_h__
#define __file_tiff_h__

#include <tiffio.h>

#include "mrtrix.h"
#include "types.h"
#include "debug.h"
#include "exception.h"
#include "file/path.h"

namespace MR
{
  namespace File
  {

    class TIFF
    { MEMALIGN (TIFF)
      public:
        TIFF (const std::string& filename, const char* mode = "r");

        ~TIFF () {
          if (tif) 
            TIFFClose (tif);
        }

        template <typename dtype>
          void read_and_check (ttag_t tag, dtype& var) {
            dtype x;
            if (TIFFGetFieldDefaulted (tif, tag, &x) != 1) 
              return;
            if (var && var != x)
              throw Exception (std::string ("mismatch between subfiles in TIFF image \"") + TIFFFileName (tif) + "\"");
            var = x;
          }

        int read_directory () {
          return TIFFReadDirectory (tif);
        }

        size_t scanline_size () const { return TIFFScanlineSize (tif); }
        void read_scanline (tdata_t buf, size_t row, size_t sample = 0) { TIFFReadScanline (tif, buf, row, sample); }

      private:
        ::TIFF* tif;

        static void error_handler (const char* module, const char* fmt, va_list ap);
    };

  }
}

#endif



