/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2013.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

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
      {

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

