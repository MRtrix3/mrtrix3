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

