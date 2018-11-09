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


#include "dwi/tractography/GT/gt.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {

        std::ostream& operator<< (std::ostream& o, Stats const& stats)
        {
          return o << stats.Tint << ", " << stats.EextTot << ", " << stats.EintTot << ", " <<
                      stats.getAcceptanceRate('b') << ", " << stats.getAcceptanceRate('d') << ", " <<
                      stats.getAcceptanceRate('r') << ", " << stats.getAcceptanceRate('o') << ", " <<
                      stats.getAcceptanceRate('c');
        }

      }
    }
  }
}

