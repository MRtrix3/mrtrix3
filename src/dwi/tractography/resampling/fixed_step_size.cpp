/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "dwi/tractography/resampling/fixed_step_size.h"

#include "math/hermite.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        bool FixedStepSize::operator() (const Streamline<>& in, Streamline<>& out) const
        {
          out.clear();
          out.index = in.index;
          out.weight = in.weight;
          Math::Hermite<value_type> interp (hermite_tension);
          // Extensions required to enable Hermite interpolation in last streamline segment at either end
          Streamline<> temp (in);
          const size_t s = temp.size();
          temp.insert    (temp.begin(), temp[0] + (temp[0] - temp[ 1 ]));
          temp.push_back (              temp[s] + (temp[s] - temp[s-1]));
          const ssize_t midpoint = temp.size()/2;
          out.push_back (temp[midpoint]);
          // Generate from the midpoint to the start, reverse, then generate from midpoint to the end
          for (ssize_t step = -1; step <= 1; step += 2) {

            ssize_t index = midpoint;
            value_type mu_lower = value_type(0);

            // Loop to generate points
            do {

              // If we don't have to step along the input track, can keep the mu from the previous
              //   interpolation point as the lower bound
              while (index > 1 && index < ssize_t(temp.size()-2) && (out.back() - temp[index+step]).norm() < step_size) {
                index += step;
                mu_lower = value_type(0);
              }
              // Always preserve the termination points, regardless of resampling
              if (index == 1) {
                out.push_back (temp[1]);
                std::reverse (out.begin(), out.end());
              } else if (index == ssize_t(temp.size()-2)) {
                out.push_back (temp[s]);
              } else {

                // Perform binary search
                point_type p_lower = temp[index], p, p_upper = temp[index+step];
                value_type mu_upper = value_type(1);
                value_type mu = value_type(0.5) * (mu_lower + mu_upper);
                do {
                  mu = value_type(0.5) * (mu_lower + mu_upper);
                  interp.set (mu);
                  p = interp.value (temp[index-step], temp[index], temp[index+step], temp[index+2*step]);
                  if ((p - out.back()).norm() < step_size) {
                    mu_lower = mu;
                    p_lower = p;
                  } else {
                    mu_upper = mu;
                    p_upper = p;
                  }
                } while ((p_upper - p_lower).norm() > value_type(0.001) * step_size);
                out.push_back (p);

              }

              // Loop until an endpoint has been added
            } while (index > 1 && index < ssize_t(temp.size()-2));

          }

          return true;
        }



      }
    }
  }
}


