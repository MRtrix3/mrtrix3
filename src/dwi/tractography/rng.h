/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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

