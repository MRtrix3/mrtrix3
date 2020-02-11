/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

