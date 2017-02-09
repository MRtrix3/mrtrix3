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


#include "math/SH.h"

namespace MR
{
  namespace Math
  {
    namespace SH
    {

      const char* encoding_description =
        "The spherical harmonic coefficients are stored as follows. First, since the "
        "signal attenuation profile is real, it has conjugate symmetry, i.e. Y(l,-m) "
        "= Y(l,m)* (where * denotes the complex conjugate). Second, the diffusion "
        "profile should be antipodally symmetric (i.e. S(x) = S(-x)), implying that "
        "all odd l components should be zero. Therefore, only the even elements are "
        "computed. \n"
        "Note that the spherical harmonics equations used here differ slightly from "
        "those conventionally used, in that the (-1)^m factor has been omitted. This "
        "should be taken into account in all subsequent calculations. \n"
        "Each volume in the output image corresponds to a different spherical harmonic "
        "component. Each volume will correspond to the following: \n"
        "volume 0: l = 0, m = 0 ; \n"
        "volume 1: l = 2, m = -2 (imaginary part of m=2 SH) ; \n"
        "volume 2: l = 2, m = -1 (imaginary part of m=1 SH) ; \n"
        "volume 3: l = 2, m = 0 ; \n"
        "volume 4: l = 2, m = 1 (real part of m=1 SH) ; \n"
        "volume 5: l = 2, m = 2 (real part of m=2 SH) ; \n"
        "etc...\n";


    }
  }
}

