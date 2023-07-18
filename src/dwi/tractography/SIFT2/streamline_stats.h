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

#include "dwi/tractography/SIFT/types.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {



      class StreamlineStats
      {

        public:
          using value_type = SIFT::value_type;

          StreamlineStats();
          StreamlineStats (const StreamlineStats&);

          StreamlineStats& operator+= (const value_type);
          StreamlineStats& operator+= (const StreamlineStats&);

          void normalise();

          value_type get_min()      const { return min; }
          value_type get_max()      const { return max; }
          value_type get_mean()     const { return mean; }
          value_type get_mean_abs() const { return mean_abs; }
          value_type get_var()      const { return var; }

          unsigned int get_count()   const { return count; }
          unsigned int get_nonzero() const { return nonzero; }

        private:
          value_type min, max;
          value_type mean, mean_abs, var;
          unsigned int count, nonzero;

      };



      }
    }
  }
}



#endif

