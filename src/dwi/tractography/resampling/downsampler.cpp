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


#include "dwi/tractography/resampling/downsampler.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        bool Downsampler::operator() (Tracking::GeneratedTrack& tck) const
        {
          if (ratio <= 1 || tck.empty())
            return false;
          size_t index_old = ratio;
          if (tck.get_seed_index()) {
            index_old = (((tck.get_seed_index() - 1) % ratio) + 1);
            tck.set_seed_index (1 + ((tck.get_seed_index() - index_old) / ratio));
          }
          size_t index_new = 1;
          while (index_old < tck.size() - 1) {
            tck[index_new++] = tck[index_old];
            index_old += ratio;
          }
          tck[index_new] = tck.back();
          tck.resize (index_new + 1);
          return true;
        }




        bool Downsampler::operator() (vector<Eigen::Vector3f>& tck) const
        {
          if (ratio <= 1 || tck.empty())
            return false;
          if (tck.size() == 1)
            return true;
          const size_t midpoint = tck.size()/2;
          size_t index_old = (((midpoint - 1) % ratio) + 1);
          size_t index_new = 1;
          while (index_old < tck.size() - 1) {
            tck[index_new++] = tck[index_old];
            index_old += ratio;
          }
          tck[index_new] = tck.back();
          tck.resize (index_new + 1);
          return true;
        }



      }
    }
  }
}


