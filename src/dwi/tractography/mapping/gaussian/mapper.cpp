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


#include "dwi/tractography/mapping/gaussian/mapper.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {
        namespace Gaussian {




          void TrackMapper::set_factor (const Streamline<>& tck, SetVoxelExtras& out) const
          {
            factors.clear();
            factors.reserve (tck.size());
            load_factors (tck);
            gaussian_smooth_factors (tck);
            out.factor = 1.0;
          }




          void TrackMapper::gaussian_smooth_factors (const Streamline<>& tck) const
          {

            std::vector<default_type> unsmoothed (factors);

            for (size_t i = 0; i != unsmoothed.size(); ++i) {

              default_type sum = 0.0, norm = 0.0;

              if (std::isfinite (unsmoothed[i])) {
                sum  = unsmoothed[i];
                norm = 1.0; // Gaussian is unnormalised -> e^0 = 1
              }

              default_type distance = 0.0;
              for (size_t j = i; j--; ) { // Decrement AFTER null test, so loop runs with j = 0
                distance += (tck[j] - tck[j+1]).norm();
                if (std::isfinite (unsmoothed[j])) {
                  const default_type this_weight = exp (-distance * distance / gaussian_denominator);
                  norm += this_weight;
                  sum  += this_weight * unsmoothed[j];
                }
              }
              distance = 0.0;
              for (size_t j = i + 1; j < unsmoothed.size(); ++j) {
                distance += (tck[j] - tck[j-1]).norm();
                if (std::isfinite (unsmoothed[j])) {
                  const default_type this_weight = exp (-distance * distance / gaussian_denominator);
                  norm += this_weight;
                  sum  += this_weight * unsmoothed[j];
                }
              }

              if (norm)
                factors[i] = (sum / norm);
              else
                factors[i] = 0.0;

            }

          }







        }
      }
    }
  }
}



