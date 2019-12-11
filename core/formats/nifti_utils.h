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

#include "header.h"


namespace MR
{
  namespace Formats
  {



    inline bool is_nifti (const Header& H)
    {
      const std::string format = H.format();
      return (format == "NIfTI-1.1" ||
              format == "NIfTI-2" ||
              format == "NIfTI-1.1 (GZip compressed)" ||
              format == "NIfTI-2 (GZip compressed)" ||
              format == "AnalyseAVW / NIfTI");
    }



  }
}

