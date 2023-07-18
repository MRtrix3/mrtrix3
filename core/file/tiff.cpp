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

#ifdef MRTRIX_TIFF_SUPPORT

#include "file/tiff.h"


namespace MR
{
  namespace File
  {

    TIFF::TIFF (const std::string& filename, const char* mode) :
      tif (nullptr) { 
        TIFFSetWarningHandler (error_handler);
        tif = TIFFOpen (filename.c_str(), mode);
        if (!tif)
          throw Exception ("error opening TIFF file \"" + filename + "\": " + strerror(errno));
      }


    void TIFF::error_handler (const char* module, const char* fmt, va_list ap)
    {
      INFO (std::string ("error in TIFF library: [") + module + "]: " + MR::printf (fmt, ap));
    }

  }
}

#endif

