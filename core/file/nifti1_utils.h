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


#ifndef __file_nifti1_utils_h__
#define __file_nifti1_utils_h__

#include "file/nifti1.h"

namespace MR
{
  class Header;

  namespace File
  {
    namespace NIfTI1
    {

      constexpr size_t header_size = 348;
      constexpr size_t header_with_ext_size = 352;

      size_t read (Header& H, const nifti_1_header& NH);
      void write (nifti_1_header& NH, const Header& H, const bool single_file);

    }
  }
}

#endif

