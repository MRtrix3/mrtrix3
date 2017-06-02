/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __file_mgh_utils_h__
#define __file_mgh_utils_h__

#include "file/mgh.h"

#define MGH_HEADER_SIZE 90
#define MGH_DATA_OFFSET 284

namespace MR
{
  class Header;

  namespace File
  {
    namespace MGH
    {

      bool read_header  (Header& H, const mgh_header& MGHH);
      void read_other   (Header& H, const mgh_other& MGHO, const bool is_BE);
      void write_header (mgh_header& MGHH, const Header& H);
      void write_other  (mgh_other&  MGHO, const Header& H);

      void write_other_to_file (const std::string&, const mgh_other&);

    }
  }
}

#endif

