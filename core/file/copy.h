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

#ifndef __file_copy_h__
#define __file_copy_h__

#include "exception.h"
#include "file/utils.h"
#include "file/mmap.h"


namespace MR
{
  namespace File
  {


    inline void copy (const std::string& source, const std::string& destination)
    {
      {
        DEBUG ("copying file \"" + source + "\" to \"" + destination + "\"...");
        MMap input (source);
        create (destination, input.size());
        MMap output (destination, true);
        ::memcpy (output.address(), input.address(), input.size());
      }
      check_app_exit_code();
    }


  }
}

#endif


