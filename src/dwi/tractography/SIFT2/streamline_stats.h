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

#ifndef __dwi_tractography_sift2_streamline_stats_h__
#define __dwi_tractography_sift2_streamline_stats_h__


#include <cassert>
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

