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


#include "dwi/tractography/resampling/fixed_step_size.h"

#include "math/hermite.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        bool FixedStepSize::operator() (vector<Eigen::Vector3f>& tck) const
        {
          Math::Hermite<float> interp (hermite_tension);
          vector<Eigen::Vector3f> output;
          // Extensions required to enable Hermite interpolation in last streamline segment at either end
          const size_t s = tck.size();
          tck.insert    (tck.begin(), tck[0] + (tck[0] - tck[ 1 ]));
          tck.push_back (             tck[s] + (tck[s] - tck[s-1]));
          const ssize_t midpoint = tck.size()/2;
          output.push_back (tck[midpoint]);
          // Generate from the midpoint to the start, reverse, then generate from midpoint to the end
          for (ssize_t step = -1; step <= 1; step += 2) {

            ssize_t index = midpoint;
            float mu_lower = 0.0f;

            // Loop to generate points
            do {

              // If we don't have to step along the input track, can keep the mu from the previous
              //   interpolation point as the lower bound
              while (index > 1 && index < ssize_t(tck.size()-2) && (output.back() - tck[index+step]).norm() < step_size) {
                index += step;
                mu_lower = 0.0f;
              }
              // Always preserve the termination points, regardless of resampling
              if (index == 1) {
                output.push_back (tck[1]);
                std::reverse (output.begin(), output.end());
              } else if (index == ssize_t(tck.size()-2)) {
                output.push_back (tck[s]);
              } else {

                // Perform binary search
                Eigen::Vector3f p_lower = tck[index], p, p_upper = tck[index+step];
                float mu_upper = 1.0f;
                float mu = 0.5 * (mu_lower + mu_upper);
                do {
                  mu = 0.5 * (mu_lower + mu_upper);
                  interp.set (mu);
                  p = interp.value (tck[index-step], tck[index], tck[index+step], tck[index+2*step]);
                  if ((p - output.back()).norm() < step_size) {
                    mu_lower = mu;
                    p_lower = p;
                  } else {
                    mu_upper = mu;
                    p_upper = p;
                  }
                } while ((p_upper - p_lower).norm() > 0.001 * step_size);
                output.push_back (p);

              }

              // Loop until an endpoint has been added
            } while (index > 1 && index < ssize_t(tck.size()-2));

          }

          std::swap (tck, output);
          return true;
        }



      }
    }
  }
}


