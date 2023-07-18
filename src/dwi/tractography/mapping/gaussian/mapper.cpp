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

            vector<default_type> unsmoothed (factors);

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



