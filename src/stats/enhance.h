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


#ifndef __stats_enhance_h__
#define __stats_enhance_h__

#include "math/stats/typedefs.h"

namespace MR
{
  namespace Stats
  {



    // This class defines the standardised interface by which statistical enhancement
    //   is performed.
    class EnhancerBase
    { NOMEMALIGN
      public:
        virtual ~EnhancerBase() { }

        // Return value is the maximal enhanced statistic
        virtual Math::Stats::value_type operator() (const Math::Stats::vector_type& /*input_statistics*/, Math::Stats::vector_type& /*enhanced_statistics*/) const = 0;

    };



  }
}

#endif
