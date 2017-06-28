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

#ifndef __image_handler_tiff_h__
#define __image_handler_tiff_h__

#ifdef MRTRIX_TIFF_SUPPORT

#include "types.h"
#include "image_io/base.h"

namespace MR
{
  namespace ImageIO
  {

    class TIFF : public Base
    { MEMALIGN (TIFF)
      public:
        TIFF (const Header& header) : Base (header) { } 

      protected:
        virtual void load (const Header&, size_t);
        virtual void unload (const Header&);
    };

  }
}

#endif
#endif


