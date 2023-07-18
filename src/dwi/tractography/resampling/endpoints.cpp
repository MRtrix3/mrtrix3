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

#include "dwi/tractography/resampling/endpoints.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        bool Endpoints::operator() (const Streamline<>& in, Streamline<>& out) const
        {
          out.clear();
          out.set_index (in.get_index());
          out.weight = in.weight;
          if (in.size() < 2)
            return true;
          out.resize (2);
          out[0] = in.front();
          out[1] = in.back();
          return true;
        }



      }
    }
  }
}


