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


#include <string>

#include "types.h"


namespace MR
{
  namespace Formats
  {



    /*! basic convenience function to determine whether an image path
     *  corresponds to a NIfTI-format image. */
    inline bool is_nifti (const std::string& path)
    {
      static const vector<std::string> exts { ".nii", ".nii.gz", ".img" };
      for (const auto& ext : exts) {
        if (path.substr (path.size() - ext.size()) == ext)
          return true;
      }
      return false;
    }



  }
}

