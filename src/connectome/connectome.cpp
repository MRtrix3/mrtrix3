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


#include "connectome/connectome.h"

#include "image.h"
#include "algo/loop.h"


namespace MR {
  namespace Connectome {



    using namespace App;
    const OptionGroup MatrixOutputOptions = OptionGroup ("Options for outputting connectome matrices")
        + Option ("symmetric", "Make matrices symmetric on output")
        + Option ("zero_diagonal", "Set matrix diagonal to zero on output");



    void check (const Header& H)
    {
      if (H.datatype().is_floating_point()) {
        // auto test = H.get_image<float>();
        // for (auto l = Loop(test) (test); l; ++l) {
        //   if (std::round (float(test.value())) != test.value())
        //     throw Exception ("Floating-point number detected in image \"" + H.name() + "\"; label images should contain integers only");
        // }
        WARN ("Image \"" + H.name() + "\" stored as floating-point; it is preferable to store label images using an unsigned integer type");
      } else if (H.datatype().is_signed()) {
        // auto test = H.get_image<int64_t>();
        // for (auto l = Loop(test) (test); l; ++l) {
        //   if (int64_t(test.value()) < int64_t(0))
        //     throw Exception ("Negative values detected in image \"" + H.name() + "\"; label images should be strictly non-negative");
        // }
        WARN ("Image \"" + H.name() + "\" stored as signed integer; it is preferable to store label images using an unsigned integer type");
      }
    }


  }
}


