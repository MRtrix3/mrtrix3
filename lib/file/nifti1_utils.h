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

#ifndef __file_nifti1_utils_h__
#define __file_nifti1_utils_h__

#include "file/nifti1.h"

namespace MR
{
  class Header;

  namespace File
  {
    namespace NIfTI
    {

      transform_type adjust_transform (const Header& H, vector<size_t>& order);

      void check (Header& H, bool single_file);
      size_t read (Header& H, const nifti_1_header& NH);
      void check (Header& H, bool single_file);
      void write (nifti_1_header& NH, const Header& H, bool single_file);

    }
  }
}

#endif

