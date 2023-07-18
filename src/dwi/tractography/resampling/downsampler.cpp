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

#include "dwi/tractography/resampling/downsampler.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        bool Downsampler::operator() (Tracking::GeneratedTrack& tck) const
        {
          if (!valid())
            return false;
          if (ratio == 1 || tck.size() <= 2)
            return true;
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
          if (!valid())
            return false;
          if (ratio == 1 || in.size() <= 2) {
            out = in;
            return true;
          }
          out.set_index (in.get_index());
          out.weight = in.weight;
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


