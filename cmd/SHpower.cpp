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

#include "app.h"
#include "progressbar.h"
#include "image/voxel.h"
#include "image/buffer.h"
#include "math/matrix.h"
#include "math/SH.h"
#include "image/loop.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage () {
  DESCRIPTION
    + "compute the power contained within each harmonic degree.";

  ARGUMENTS
    + Argument ("SH", "the input spherical harmonics coefficients image.").type_image_in ()
    + Argument ("power", "the ouput power image.").type_image_out ();
}


void run () {
  Image::Buffer<float> SH_data (argument[0]);
  Image::Header power_header (SH_data);

  if (power_header.ndim() != 4)
    throw Exception ("SH image should contain 4 dimensions");


  int lmax = Math::SH::LforN (SH_data.dim (3));
  inform ("calculating spherical harmonic power up to degree " + str (lmax));

  power_header.dim (3) = 1 + lmax/2;
  power_header.datatype() = DataType::Float32;

  Image::Buffer<float>::voxel_type SH (SH_data);

  Image::Buffer<float> power_data (argument[1], power_header);
  Image::Buffer<float>::voxel_type P (power_data);

  Image::LoopInOrder loop (P, "calculating SH power...", 0, 3);
  for (loop.start (P, SH); loop.ok(); loop.next (P, SH)) {
    P[3] = 0;
    for (int l = 0; l <= lmax; l+=2) {
      P.value() = 0.0;
      for (int m = -l; m <= l; ++m) {
        SH[3] = Math::SH::index (l, m);
        P.value() += Math::pow2 (float (SH.value()));
      }
      ++P[3];
    }
  }

}
