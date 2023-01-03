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

#ifndef __math_Sn_scale_estimator_h__
#define __math_Sn_scale_estimator_h__

#include "types.h"
#include "math/median.h"


namespace MR {
  namespace Math {


    // Sn robust estimator of scale to get solid estimate of standard deviation:
    // for details, see: Rousseeuw PJ, Croux C. Alternatives to the Median Absolute Deviation. Journal of the American Statistical Association 1993;88:1273–1283. 

    template <typename value_type = default_type> 
      class Sn_scale_estimator { NOMEMALIGN
        public:
          template <class VectorType>
            value_type operator() (const VectorType& vec)
            {
              diff.resize (vec.size());
              med_diff.resize (vec.size());
              for (ssize_t j = 0; j < vec.size(); ++j) {
                for (ssize_t i = 0; i < vec.size(); ++i) 
                  diff[i] = abs (vec[i] - vec[j]);
                med_diff[j] = Math::median (diff);
              }
              return 1.1926 * Math::median (med_diff);
            }

        protected:
          vector<value_type> diff, med_diff;

      };

  }
}

#endif


