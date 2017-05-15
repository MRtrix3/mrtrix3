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


#include "dwi/tractography/SIFT/track_contribution.h"

namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {


        float Track_fixel_contribution::scale_to_storage = 0.0;
        float Track_fixel_contribution::scale_from_storage = 0.0;
        float Track_fixel_contribution::min_length_for_storage = 0.0;


      }
    }
  }
}


