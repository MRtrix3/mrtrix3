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

#ifndef __sparse_legacy_keys_h__
#define __sparse_legacy_keys_h__

#include <string>

namespace MR
{
  namespace Sparse
  {

    namespace Legacy
    {
      // These are the keys that must be present in an image header to successfully read or write sparse image data
      const std::string name_key ("sparse_data_name");
      const std::string size_key ("sparse_data_size");
    }
  }
}

#endif




