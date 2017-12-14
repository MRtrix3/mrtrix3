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


#ifndef __dwi_tractography_resampling_endpoints_h__
#define __dwi_tractography_resampling_endpoints_h__


#include "dwi/tractography/resampling/resampling.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        class Endpoints : public BaseCRTP<Endpoints>
        { NOMEMALIGN

          public:
            Endpoints() { }

            bool operator() (const Streamline<>&, Streamline<>&) const override;
            bool valid() const override { return true; }

        };



      }
    }
  }
}

#endif



