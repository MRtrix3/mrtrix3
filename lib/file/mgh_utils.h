/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 29/09/12.

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

