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



#include "command.h"
#include "image.h"
#include "math/SH.h"
#include "algo/threaded_loop.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  DESCRIPTION
    + "compute the total power of a spherical harmonics image."
    
    + "This command computes the sum of squared SH coefficients, "
      "which equals the mean-squared amplitude "
      "of the spherical function it represents.";

  ARGUMENTS
    + Argument ("SH", "the input spherical harmonics coefficients image.").type_image_in ()
    + Argument ("power", "the output power image.").type_image_out ();
  
  OPTIONS
    + Option ("spectrum", "output the power spectrum, i.e., the power contained within each harmonic degree (l=0, 2, 4, ...) as a 4-D image.");
  
}


void run () {
  auto SH_data = Image<float>::open(argument[0]);
  Math::SH::check (SH_data);

  Header power_header (SH_data);
  
  bool spectrum = get_options("spectrum").size();

  int lmax = Math::SH::LforN (SH_data.size (3));
  INFO ("calculating spherical harmonic power up to degree " + str (lmax));

  if (spectrum)
    power_header.size (3) = 1 + lmax/2;
  else
    power_header.ndim() = 3;
  power_header.datatype() = DataType::Float32;

  auto power_data = Image<float>::create(argument[1], power_header);

  auto f1 = [&] (decltype(power_data)& P, decltype(SH_data)& SH) {
    P.index(3) = 0;
    for (int l = 0; l <= lmax; l+=2) {
      float power = 0.0;
      for (int m = -l; m <= l; ++m) {
        SH.index(3) = Math::SH::index (l, m);
        float val = SH.value();
#ifdef USE_NON_ORTHONORMAL_SH_BASIS
        if (m != 0) 
          val *= Math::sqrt1_2;
#endif
        power += Math::pow2 (val);
      }
      P.value() = power / (Math::pi * 4);
      ++P.index(3);
    }
  };
  
  auto f2 = [&] (decltype(power_data)& P, decltype(SH_data)& SH) {
    float power = 0.0;
    for (int l = 0; l <= lmax; l+=2) {
      for (int m = -l; m <= l; ++m) {
        SH.index(3) = Math::SH::index (l, m);
        float val = SH.value();
#ifdef USE_NON_ORTHONORMAL_SH_BASIS
        if (m != 0) 
          val *= Math::sqrt1_2;
#endif
        power += Math::pow2 (val);
      }
    }
    P.value() = power / (Math::pi * 4);
  };
  
  auto loop = ThreadedLoop ("calculating SH power", SH_data, 0, 3);
  if (spectrum)
    loop.run(f1, power_data, SH_data);
  else
    loop.run(f2, power_data, SH_data);
  
}
