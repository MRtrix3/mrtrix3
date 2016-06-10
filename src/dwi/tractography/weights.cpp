/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */
#include "dwi/tractography/weights.h"

namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {

      using namespace App;

      const Option TrackWeightsInOption
      = Option ("tck_weights_in", "specify a text scalar file containing the streamline weights")
          + Argument ("path").type_file_in();

      const Option TrackWeightsOutOption
      = Option ("tck_weights_out", "specify the path for an output text scalar file containing streamline weights")
          + Argument ("path").type_file_out();

    }
  }
}


