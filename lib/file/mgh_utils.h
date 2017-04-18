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

#ifndef __file_mgh_utils_h__
#define __file_mgh_utils_h__

#include "raw.h"
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

      inline mgh_tag prepare_tag (int32_t id, int64_t size) {
        mgh_tag tag;
        tag.id = ByteOrder::BE (id);
        tag.size = ByteOrder::BE (size);
        return tag;
      }

      void read_header  (Header& H, const mgh_header& MGHH);
      void read_other   (Header& H, const mgh_other& MGHO);
      void read_tag     (Header& H, const mgh_tag& tag);

      void write_header (mgh_header& MGHH, const Header& H);
      void write_other  (mgh_other&  MGHO, const Header& H);

    }
  }
}

#endif

