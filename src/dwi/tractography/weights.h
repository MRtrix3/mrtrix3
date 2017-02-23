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


#ifndef __dwi_tractography_weights_h__
#define __dwi_tractography_weights_h__
#include "__mrtrix_plugin.h"

#include "cmdline_option.h"

namespace MR
{
  namespace DWI
  {

    namespace Tractography
    {

      extern const App::Option TrackWeightsInOption;
      extern const App::Option TrackWeightsOutOption;

    }
  }
}

#endif

