/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __stats_tfce_h__
#define __stats_tfce_h__

#include "thread_queue.h"
#include "filter/connected_components.h"
#include "math/stats/permutation.h"
#include "math/stats/typedefs.h"

namespace MR
{
  namespace Stats
  {
    namespace TFCE
    {



      typedef Math::Stats::value_type value_type;
      typedef Math::Stats::vector_type vector_type;



      /** \addtogroup Statistics
      @{ */
      class Enhancer {
        public:
          Enhancer (const Filter::Connector& connector, const value_type dh, const value_type E, const value_type H);

          value_type operator() (const value_type max_stat, const vector_type& stats, vector_type& enhanced_stats) const;

        protected:
          const Filter::Connector& connector;
          const value_type dh, E, H;
      };
      //! @}



    }
  }
}

#endif
