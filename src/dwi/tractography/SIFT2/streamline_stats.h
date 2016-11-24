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


#ifndef __dwi_tractography_sift2_streamline_stats_h__
#define __dwi_tractography_sift2_streamline_stats_h__


#include <assert.h>
#include <limits>

#include "math/math.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {



      class StreamlineStats
      { NOMEMALIGN

        public:
          StreamlineStats();
          StreamlineStats (const StreamlineStats&);

          StreamlineStats& operator+= (const double);
          StreamlineStats& operator+= (const StreamlineStats&);

          void normalise();

          double get_min()      const { return min; }
          double get_max()      const { return max; }
          double get_mean()     const { return mean; }
          double get_mean_abs() const { return mean_abs; }
          double get_var()      const { return var; }

          unsigned int get_count()   const { return count; }
          unsigned int get_nonzero() const { return nonzero; }

        private:
          double min, max;
          double mean, mean_abs, var;
          unsigned int count, nonzero;

      };



      }
    }
  }
}



#endif

