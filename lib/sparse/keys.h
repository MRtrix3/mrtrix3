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

#ifndef __sparse_keys_h__
#define __sparse_keys_h__

#include <string>

namespace MR
{
  namespace Sparse
  {
    const std::string n_fixels_key ("nfixels");
    const std::initializer_list <const std::string> supported_sparse_formats { ".mif", ".nii", ".mif.gz" , ".nii.gz" };
  }
}

#endif
