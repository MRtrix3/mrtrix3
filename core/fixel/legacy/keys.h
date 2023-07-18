/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#ifndef __fixel_legacy_keys_h__
#define __fixel_legacy_keys_h__

#include <string>

namespace MR
{
  namespace Fixel
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




