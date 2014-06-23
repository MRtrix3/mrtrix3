/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 27/06/08.

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

#ifndef __dwi_Sn_scale_estimator_h__
#define __dwi_Sn_scale_estimator_h__

#include "math/vector.h"
#include "math/median.h"


namespace MR {
  namespace DWI {


    // Sn robust estimator of scale to get solid estimate of standard deviation:
    // for details, see: Rousseeuw PJ, Croux C. Alternatives to the Median Absolute Deviation. Journal of the American Statistical Association 1993;88:1273–1283. 

    template <typename value_type> 
      class Sn_scale_estimator {
        public:
          template <class Container>
            value_type operator() (const Container& vec)
            {
              diff.resize (vec.size());
              med_diff.resize (vec.size());
              for (size_t j = 0; j < vec.size(); ++j) {
                for (size_t i = 0; i < vec.size(); ++i) 
                  diff[i] = Math::abs (vec[i] - vec[j]);
                med_diff[j] = Math::median (diff);
              }
              return 1.1926 * Math::median (med_diff);
            }

        protected:
          std::vector<value_type> diff, med_diff;

      };

  }
}

#endif


