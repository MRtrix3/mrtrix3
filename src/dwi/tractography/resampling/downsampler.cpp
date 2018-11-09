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




        bool Downsampler::operator() (const Streamline<>& in, Streamline<>& out) const
        {
          out.clear();
          out.index = in.index;
          out.weight = in.weight;
          if (ratio <= 1 || in.empty())
            return false;
          if (in.size() == 1)
            return true;
          out.push_back (in.front());
          const size_t midpoint = in.size()/2;
          size_t index = (((midpoint - 1) % ratio) + 1);
          while (index < in.size() - 1) {
            out.push_back (in[index]);
            index += ratio;
          }
          out.push_back (in.back());
          return true;
        }



      }
    }
  }
}


