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

#include "header.h"
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
        CONSOLE ("Image \"" + H.name() + "\" stored with floating-point type; need to check for non-integer or negative values");
        // Need to open the image WITHOUT using the IO handler stored in H;
        //   creating an image from this "claims" the handler from the header, and
        //   therefore once this check has completed the image can no longer be opened
        auto test = Image<float>::open (H.name());
        for (auto l = Loop("Verifying parcellation image", test) (test); l; ++l) {
          if (std::round (float(test.value())) != test.value())
            throw Exception ("Floating-point number detected in image \"" + H.name() + "\"; label images should contain integers only");
          if (float(test.value()) < 0.0f)
            throw Exception ("Negative value detected in image \"" + H.name() + "\"; label images should be strictly non-negative");
        }
        //WARN ("Image \"" + H.name() + "\" stored as floating-point; it is preferable to store label images using an unsigned integer type");
      } else if (H.datatype().is_signed()) {
        CONSOLE ("Image \"" + H.name() + "\" stored with signed integer type; need to check for negative values");
        auto test = Image<int64_t>::open (H.name());
        for (auto l = Loop("Verifying parcellation image", test) (test); l; ++l) {
          if (int64_t(test.value()) < int64_t(0))
            throw Exception ("Negative value detected in image \"" + H.name() + "\"; label images should be strictly non-negative");
        }
        //WARN ("Image \"" + H.name() + "\" stored as signed integer; it is preferable to store label images using an unsigned integer type");
      }
    }


  }
}


