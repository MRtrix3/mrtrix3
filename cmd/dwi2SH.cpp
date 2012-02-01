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
#include "image/data.h"
#include "image/voxel.h"
#include "math/matrix.h"
#include "math/SH.h"
#include "dwi/gradient.h"
#include "image/loop.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "convert base diffusion-weighted images to their spherical harmonic representation."

  + "This program outputs the spherical harmonic decomposition for the set"
  "measured signal attenuations. The signal attenuations are calculated by "
  "identifying the b-zero images from the diffusion encoding supplied (i.e. "
  "those with zero as the b-value), and dividing the remaining signals by "
  "the mean b-zero signal intensity. The spherical harmonic decomposition is "
  "then calculated by least-squares linear fitting."

  + "Note that this program makes use of implied symmetries in the diffusion "
  "profile. First, the fact the signal attenuation profile is real implies "
  "that it has conjugate symmetry, i.e. Y(l,-m) = Y(l,m)* (where * denotes the "
  "complex conjugate). Second, the diffusion profile should be antipodally "
  "symmetric (i.e. S(x) = S(-x)), implying that all odd l components should be "
  "zero. Therefore, this program only computes the even elements."

  + "Note that the spherical harmonics equations used here differ slightly from "
  "those conventionally used, in that the (-1)^m factor has been omitted. This "
  "should be taken into account in all subsequent calculations."

  + Math::SH::encoding_description;

  ARGUMENTS
  + Argument ("dwi", "the input diffusion-weighted image.").type_image_in ()
  + Argument ("SH", "the output spherical harmonics coefficients image.").type_image_out ();


  OPTIONS
  + Option ("lmax",
            "set the maximum harmonic order for the output series. By default, the "
            "program will use the highest possible lmax given the number of "
            "diffusion-weighted images.")
  + Argument ("order").type_integer (2, 8, 30)

  + Option ("normalise", "normalise the DW signal to the b=0 image")

  + DWI::GradOption;
}



void run ()
{
  Image::Data<float> dwi_data (argument[0]);

  Math::Matrix<float> grad = DWI::get_DW_scheme<float> (dwi_data);

  std::vector<int> bzeros, dwis;
  DWI::guess_DW_directions (dwis, bzeros, grad);

  Options opt = get_options ("lmax");
  int lmax = opt.size() ? opt[0][0] : Math::SH::LforN (dwis.size());
  if (lmax > int (Math::SH::LforN (dwis.size()))) {
    info ("warning: not enough data to estimate spherical harmonic components up to order " + str (lmax));
    lmax = Math::SH::LforN (dwis.size());
  }
  info ("calculating even spherical harmonic components up to order " + str (lmax));

  Math::Matrix<float> dirs;
  DWI::gen_direction_matrix (dirs, grad, dwis);
  Math::SH::Transform<float> SHT (dirs, lmax);

  bool normalise = get_options ("normalise").size();

  Image::Header header (dwi_data);
  header.dim (3) = Math::SH::NforL (lmax);
  header.datatype() = DataType::Float32;
  Image::Data<float> SH_data (header, argument[1]);

  Image::Data<float>::voxel_type dwi (dwi_data);
  Image::Data<float>::voxel_type SH (SH_data);

  Math::Vector<float> res (lmax);
  Math::Vector<float> sigs (dwis.size());

  Image::LoopInOrder loop (SH, "converting DW images to SH coefficients...", 0, 3);
  for (loop.start (SH, dwi); loop.ok(); loop.next (SH, dwi)) {

    double norm = 0.0;
    if (normalise) {
      for (size_t n = 0; n < bzeros.size(); n++) {
        dwi[3] = bzeros[n];
        norm += dwi.value ();
      }
      norm /= bzeros.size();
    }

    for (size_t n = 0; n < dwis.size(); n++) {
      dwi[3] = dwis[n];
      sigs[n] = dwi.value();
      if (sigs[n] < 0.0) sigs[n] = 0.0;
      if (normalise) sigs[n] /= norm;
    }

    SHT.A2SH (res, sigs);

    for (SH[3] = 0; SH[3] < SH.dim (3); ++SH[3])
      SH.value() = res[SH[3]];
  }

}
