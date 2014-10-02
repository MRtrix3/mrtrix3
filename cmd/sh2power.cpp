/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "command.h"
#include "progressbar.h"
#include "image/voxel.h"
#include "image/buffer.h"
#include "math/matrix.h"
#include "math/SH.h"
#include "image/loop.h"


using namespace MR;
using namespace App;

void usage () {
  DESCRIPTION
    + "compute the power contained within each harmonic degree.";

  ARGUMENTS
    + Argument ("SH", "the input spherical harmonics coefficients image.").type_image_in ()
    + Argument ("power", "the output power image.").type_image_out ();
}


void run () {
  Image::Buffer<float> SH_data (argument[0]);
  Image::Header power_header (SH_data);

  if (power_header.ndim() != 4)
    throw Exception ("SH image should contain 4 dimensions");


  int lmax = Math::SH::LforN (SH_data.dim (3));
  INFO ("calculating spherical harmonic power up to degree " + str (lmax));

  power_header.dim (3) = 1 + lmax/2;
  power_header.datatype() = DataType::Float32;

  auto SH_vox = SH_data.voxel();

  Image::Buffer<float> power_data (argument[1], power_header);
  auto power_vox = power_data.voxel();

  auto f = [&] (decltype(power_vox)& P, decltype(SH_vox)& SH) {
    P[3] = 0;
    for (int l = 0; l <= lmax; l+=2) {
      float power = 0.0;
      for (int m = -l; m <= l; ++m) {
        SH[3] = Math::SH::index (l, m);
        float val = SH.value();
#ifdef USE_NON_ORTHONORMAL_SH_BASIS
        if (m != 0) 
          val *= Math::sqrt1_2;
#endif
        power += Math::pow2 (val);
      }
      P.value() = power / float (2*l+1);
      ++P[3];
    }
  };
  Image::ThreadedLoop ("calculating SH power...", SH_vox, 0, 3)
    .run (f, power_vox, SH_vox);
}
