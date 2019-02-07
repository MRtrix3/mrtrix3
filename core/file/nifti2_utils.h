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

#ifndef __file_nifti2_utils_h__
#define __file_nifti2_utils_h__

#include "file/nifti2.h"

namespace MR
{
  class Header;

  namespace File
  {
    namespace NIfTI2
    {

      constexpr size_t header_size = 540;
      constexpr size_t header_with_ext_size = 544;
      constexpr char signature_extra[4] { '\r', '\n', '\032', '\n' };

      size_t read (Header& H, const nifti_2_header& NH);
      void write (nifti_2_header& NH, const Header& H, const bool single_file);

    }
  }
}

#endif

