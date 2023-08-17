/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "types.h"

#include "fixel/correspondence/correspondence.h"

//#define FIXELCORRESPONDENCE_TEST_DP2COST


namespace MR {
  namespace Fixel {
    namespace Correspondence {



      // Fast lookup for angular penalisation term
      class DP2Cost
      {
        public:
          DP2Cost () :
              data (dp2cost_lookup_resolution+2),
              multiplier (dp2cost_lookup_resolution)
          {
            for (size_t bin = 0; bin <= dp2cost_lookup_resolution; ++bin) {
              const default_type dp = default_type(bin) / default_type(dp2cost_lookup_resolution);
              data[bin] = std::tan(std::acos(dp));
            }
            // Pad lookup table so that the functor does not need to branch
            //   to prevent buffer overrun in the case where dp = 1.0
            data[dp2cost_lookup_resolution+1] = 0.0f;
          }

          float operator() (const float dp) const
          {
            assert (dp >= 0.0f && dp <= 1.0f);
            const float position = dp * multiplier;
            const size_t lower = std::floor (position);
            const float mu = position - float(lower);
            const float result = ((1.0f-mu) * data[lower]) + (mu * data[lower+1]);
#ifdef FIXELCORRESPONDENCE_TEST_DP2COST
            std::cerr << "DP = " << dp << "; exact = " << std::tan (std::acos (dp)) << "; lookup = " << result << "\n";
#endif
            return result;
          }
        private:
          Eigen::Array<float, Eigen::Dynamic, 1> data;
          const float multiplier;
      };



    }
  }
}
