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
 * See the Mozila Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __dwi_tractography_rng_h__
#define __dwi_tractography_rng_h__

#include "math/rng.h"

namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {

      //! thread-local, but globally accessible RNG to vastly simplify multi-threading
#ifdef MRTRIX_MACOSX
      extern __thread Math::RNG* rng;
#else
      extern thread_local Math::RNG* rng;
#endif 

    }
  }
}


#endif

