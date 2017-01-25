/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#include "dwi/tractography/resampling/fixed_num_points.h"

#include "math/hermite.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        bool FixedNumPoints::operator() (std::vector<Eigen::Vector3f>& tck) const
        {
          // Perform an explicit calculation of streamline length
          // From this, derive the spline position of each sample
          assert (tck.size() > 1);
          float length = 0.0;
          std::vector<float> steps;
          for (size_t i = 1; i != tck.size(); ++i) {
            const float dist = (tck[i] - tck[i-1]).norm();
            length += dist;
            steps.push_back (dist);
          }
          steps.push_back (0.0f);

          Math::Hermite<float> interp (hermite_tension);
          std::vector<Eigen::Vector3f> output;
          const size_t s = tck.size();
          tck.insert    (tck.begin(), tck[0] + (tck[0] - tck[ 1 ]));
          tck.push_back (             tck[s] + (tck[s] - tck[s-1]));

          float cumulative_length = 0.0;
          size_t input_index = 0;
          for (size_t output_index = 0; output_index != num_points; ++output_index) {
            const float target_length = length * output_index / float(num_points-1);
            while (input_index < s && (cumulative_length + steps[input_index] < target_length))
              cumulative_length += steps[input_index++];
            if (input_index == s) {
              output.push_back (tck[s]);
              break;
            }
            const float mu = (target_length - cumulative_length) / steps[input_index];
            interp.set (mu);
            output.push_back (interp.value (tck[input_index], tck[input_index+1], tck[input_index+2], tck[input_index+3]));
          }

          std::swap (tck, output);
          return true;
        }



      }
    }
  }
}


