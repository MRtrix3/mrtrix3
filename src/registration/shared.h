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


#ifndef __registration_shared_h__
#define __registration_shared_h__

#include "app.h"

namespace MR
{
  namespace Registration
  {
    using namespace MR;
    using namespace App;

    const OptionGroup multiContrastOptions =
      OptionGroup ("Multi-contrast options")
      + Option ("mc_weights", "relative weight of images used for multi-contrast registration. Default: 1.0 (equal weighting)")
        + Argument ("weights").type_sequence_float ();

    const OptionGroup fod_options =
          OptionGroup ("FOD registration options")

          + Option ("directions", "the directions used for FOD reorienation using apodised point spread functions (Default: 60 directions)")
          + Argument ("file", "a list of directions [az el] generated using the gendir command.").type_file_in ()

          + Option ("noreorientation", "turn off FOD reorientation. Reorientation is on by default if the number "
                                       "of volumes in the 4th dimension corresponds to the number of coefficients in an "
                                       "antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc");
  }
}

#endif
