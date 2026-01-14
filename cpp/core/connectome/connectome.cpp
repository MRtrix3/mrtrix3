/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include "connectome/connectome.h"

#include "algo/loop.h"
#include "header.h"
#include "image.h"
#include "mrtrix.h"

namespace MR::Connectome {

using namespace App;
// clang-format off
const OptionGroup MatrixOutputOptions =
    OptionGroup("Options for outputting connectome matrices")
    + Option("symmetric", "Make matrices symmetric on output")
    + Option("zero_diagonal", "Set matrix diagonal to zero on output");
// clang-format on

void check(const Header &H) {
  if (!(H.ndim() == 3 || (H.ndim() == 4 && H.size(3) == 1)))
    throw Exception("Image \"" + std::string(H.name()) + "\" is not 3D," + //
                    " and hence is not a volume of node parcel indices");  //
  if (H.datatype().is_floating_point()) {
    CONSOLE("Image \"" + std::string(H.name()) + "\" stored with floating-point type;" + //
            " need to check for non-integer or negative values");                        //
    // Need to open the image WITHOUT using the IO handler stored in H;
    //   creating an image from this "claims" the handler from the header, and
    //   therefore once this check has completed the image can no longer be opened
    auto test = Image<float>::open(H.name());
    for (auto l = Loop("Verifying parcellation image", test)(test); l; ++l) {
      if (std::round(static_cast<float>(test.value())) != test.value())
        throw Exception("Floating-point number detected in image \"" + std::string(H.name()) + "\";" + //
                        " label images should contain integers only");                                 //
      if (static_cast<float>(test.value()) < 0.0F)
        throw Exception("Negative value detected in image \"" + std::string(H.name()) + "\";" + //
                        " label images should be strictly non-negative");                       //
    }
    // WARN ("Image \"" + H.name() + "\" stored as floating-point; it is preferable to store label images using an
    // unsigned integer type");
  } else if (H.datatype().is_signed()) {
    CONSOLE("Image \"" + std::string(H.name()) + "\" stored with signed integer type;" + //
            " need to check for negative values");                                       //
    auto test = Image<int64_t>::open(H.name());
    for (auto l = Loop("Verifying parcellation image", test)(test); l; ++l) {
      if (static_cast<int64_t>(test.value()) < int64_t(0))
        throw Exception("Negative value detected in image \"" + std::string(H.name()) + "\";" + //
                        " label images should be strictly non-negative");                       //
    }
    // WARN ("Image \"" + H.name() + "\" stored as signed integer; it is preferable to store label images using an
    // unsigned integer type");
  }
}

} // namespace MR::Connectome
