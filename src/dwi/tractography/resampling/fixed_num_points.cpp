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

#include "dwi/tractography/resampling/fixed_num_points.h"

#include "math/hermite.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        bool FixedNumPoints::operator() (const Streamline<>& in, Streamline<>& out) const
        {
          // Perform an explicit calculation of streamline length
          // From this, derive the spline position of each sample
          out.clear();
          if (!valid())
            return false;
          out.set_index (in.get_index());
          out.weight = in.weight;
          if (in.size() < 2)
            return true;
          value_type length = 0.0;
          vector<value_type> steps;
          for (size_t i = 1; i != in.size(); ++i) {
            const value_type dist = (in[i] - in[i-1]).norm();
            length += dist;
            steps.push_back (dist);
          }
          steps.push_back (value_type(0));

          Math::Hermite<value_type> interp (hermite_tension);
          Streamline<> temp (in);
          const size_t s = temp.size();
          temp.insert    (temp.begin(), temp[0] + (temp[0] - temp[ 1 ]));
          temp.push_back (              temp[s] + (temp[s] - temp[s-1]));

          value_type cumulative_length = value_type(0);
          size_t input_index = 0;
          for (size_t output_index = 0; output_index != num_points; ++output_index) {
            const value_type target_length = length * output_index / value_type(num_points-1);
            while (input_index < s && (cumulative_length + steps[input_index] < target_length))
              cumulative_length += steps[input_index++];
            if (input_index == s) {
              out.push_back (temp[s]);
              break;
            }
            const value_type mu = steps[input_index] ?
                                  (target_length - cumulative_length) / steps[input_index] :
                                  0.5;
            interp.set (mu);
            out.push_back (interp.value (temp[input_index], temp[input_index+1], temp[input_index+2], temp[input_index+3]));
            assert (out.back().allFinite());
          }

          return true;
        }



      }
    }
  }
}


