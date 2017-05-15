/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#include "dwi/tractography/resampling/endpoints.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        bool Endpoints::operator() (const Streamline<>& in, Streamline<>& out) const
        {
          out.resize (2);
          out.index = in.index;
          out.weight = in.weight;
          out[0] = in.front();
          out[1] = in.back();
          return true;
        }



      }
    }
  }
}


