/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
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

