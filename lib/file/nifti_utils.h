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

#include <vector>

#include "types.h"

namespace MR
{
  class Header;

  namespace File
  {
    namespace NIfTI
    {

      bool right_left_warning_issued = false;

      transform_type adjust_transform (const Header& H, std::vector<size_t>& order);

      void check (Header& H, const bool is_analyse);
      size_t version (Header& H);

    }
  }
}

#endif

