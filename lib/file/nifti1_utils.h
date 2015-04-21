/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 26/08/09.

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

#ifndef __file_nifti1_utils_h__
#define __file_nifti1_utils_h__

#include "file/nifti1.h"
#include "math/matrix.h"

namespace MR
{
  class Header;

  namespace File
  {
    namespace NIfTI
    {

      Math::Matrix<default_type> adjust_transform (const Header& H, std::vector<size_t>& order);

      void check (Header& H, bool single_file);
      //! \todo add straight Analyse support
      size_t read (Header& H, const nifti_1_header& NH);
      void check (Header& H, bool single_file);
      //! \todo need to double-check new transform handling code
      void write (nifti_1_header& NH, const Header& H, bool single_file);

    }
  }
}

#endif

