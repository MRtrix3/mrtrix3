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

#ifndef __file_nifti_utils_h__
#define __file_nifti_utils_h__

#include "file/nifti1.h"
#include "file/nifti2.h"

namespace MR
{
  class Header;

  namespace File
  {
    namespace NIfTI
    {

      constexpr size_t ver1_hdr_size = 348;
      constexpr size_t ver1_hdr_with_ext_size = 352;
      constexpr size_t ver2_hdr_size = 540;
      constexpr size_t ver2_hdr_with_ext_size = 544;
      constexpr char ver2_sig_extra[4] { '\r', '\n', '\032', '\n' };

      transform_type adjust_transform (const Header& H, std::vector<size_t>& order);

      size_t read (Header& H, const nifti_1_header& NH);
      size_t read (Header& H, const nifti_2_header& NH);
      void check (Header& H, bool single_file);
      size_t version (Header& H);
      void write (nifti_1_header& NH, const Header& H, bool single_file);
      void write (nifti_2_header& NH, const Header& H, bool single_file);

    }
  }
}

#endif

