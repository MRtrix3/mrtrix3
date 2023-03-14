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

#ifndef __dwi_dwi_h__
#define __dwi_dwi_h__


#include "file/config.h"


// Default b-value threshold for a shell / volume to be classified as "b=0"
#define DWI_SHELLS_BZERO_THREHSOLD 10.0



//CONF option: BZeroThreshold
//CONF default: 10.0
//CONF Specifies the b-value threshold for determining those image
//CONF volumes that correspond to b=0.



namespace MR
{

  namespace DWI
  {

    FORCE_INLINE default_type bzero_threshold () {
      static const default_type value = File::Config::get_float ("BZeroThreshold", DWI_SHELLS_BZERO_THREHSOLD);
      return value;
    }

  }
}

#endif

