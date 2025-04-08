/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#ifndef __metadata_slice_encoding_h__
#define __metadata_slice_encoding_h__

#include <string>

#include "types.h"

namespace MR {
  class Header;
}

namespace MR {
  namespace Metadata {
    namespace SliceEncoding {

      void transform_for_image_load(KeyValues& keyval, const Header& H);

      void transform_for_nifti_write(KeyValues& keyval, const Header& H);

      std::string resolve_slice_timing(const std::string& one, const std::string& two);

      void clear(KeyValues&);

    }
  }
}

#endif
