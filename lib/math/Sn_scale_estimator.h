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

#ifndef __math_Sn_scale_estimator_h__
#define __math_Sn_scale_estimator_h__

#include <vector>

#include "types.h"
#include "math/median.h"


namespace MR {
  namespace Math {


    // Sn robust estimator of scale to get solid estimate of standard deviation:
    // for details, see: Rousseeuw PJ, Croux C. Alternatives to the Median Absolute Deviation. Journal of the American Statistical Association 1993;88:1273–1283. 

    template <typename value_type = default_type> 
      class Sn_scale_estimator {
        public:
          template <class VectorType>
            value_type operator() (const VectorType& vec)
            {
              diff.resize (vec.size());
              med_diff.resize (vec.size());
              for (ssize_t j = 0; j < vec.size(); ++j) {
                for (ssize_t i = 0; i < vec.size(); ++i) 
                  diff[i] = std::abs (vec[i] - vec[j]);
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


