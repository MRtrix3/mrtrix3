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


#ifndef __fixel_fixel_h__
#define __fixel_fixel_h__

#include <string>
#include <cstdint>

namespace MR
{
  namespace Fixel
  {


    //! a string containing a description of the fixel directory format
    /*! This can used directly in the DESCRIPTION field of a command's
     * usage() function. */
    extern const char* format_description;

    using index_type = uint32_t;

    const std::string n_fixels_key ("nfixels");
    const std::initializer_list <const std::string> supported_sparse_formats { ".mif", ".nii", ".mif.gz" , ".nii.gz" };


  }
}

#endif
