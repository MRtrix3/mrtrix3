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
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/voxel.h"
#include "math/matrix.h"
#include "math/SH.h"
#include "dwi/gradient.h"
#include "image/loop.h"


using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "convert a set of amplitudes (defined along a set of corresponding directions) "
    "to their spherical harmonic representation. The spherical harmonic decomposition is "
    "calculated by least-squares linear fitting."

  + "The directions can be defined either as a DW gradient scheme (for example to compute "
    "the SH representation of the DW signal) or a set of [az el] pairs as output by the gendir "
    "command. The DW gradient scheme or direction set can be supplied within the input "
    "image header or using the -gradient or -directions option. Note that if a direction set "
    "and DW gradient scheme can be found, the direction set will be used by default."

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
  + Argument ("amp", "the input amplitude image.").type_image_in ()
  + Argument ("SH", "the output spherical harmonics coefficients image.").type_image_out ();


  OPTIONS
  + Option ("lmax",
            "set the maximum harmonic order for the output series. By default, the "
            "program will use the highest possible lmax given the number of "
            "diffusion-weighted images.")
  + Argument ("order").type_integer (0, 8, 30)

  + Option ("normalise", "normalise the DW signal to the b=0 image")

  + Option ("directions", "the directions corresponding to the input amplitude image used to sample AFD. "
                          "By default this option is not required providing the direction set is supplied "
                          "in the amplitude image.")
  + Argument ("file", "a list of directions [az el] generated using the gendir command.").type_file()

  + DWI::GradOption

  + Image::Stride::StrideOption;
}



void run ()
{
  std::vector<ssize_t> strides (4, 0);
  strides[3] = 1;
  Image::BufferPreload<float> amp_data (argument[0], strides);
  Image::Header header (amp_data);

  std::vector<int> bzeros, dwis;
  Math::Matrix<float> dirs;
  Options opt = get_options ("directions");
  if (opt.size()) {
    dirs.load(opt[0][0]);
  } else {
    if (header["directions"].size()) {
      std::vector<float> dir_vector;
      std::vector<std::string > lines = split (header["directions"], "\n", true);
      for (size_t l = 0; l < lines.size(); l++) {
        std::vector<float> v (parse_floats(lines[l]));
        dir_vector.insert (dir_vector.end(), v.begin(), v.end());
      }
      dirs.resize(dir_vector.size() / 2, 2);
      for (size_t i = 0; i < dir_vector.size(); i += 2) {
        dirs(i / 2, 0) = dir_vector[i];
        dirs(i / 2, 1) = dir_vector[i + 1];
      }
    } else {
      Math::Matrix<float> grad = DWI::get_valid_DW_scheme<float> (amp_data);
      DWI::guess_DW_directions (dwis, bzeros, grad);
      DWI::gen_direction_matrix (dirs, grad, dwis);
    }
  }


  opt = get_options ("lmax");
  int lmax = opt.size() ? opt[0][0] : Math::SH::LforN (dirs.rows());
  if (lmax > int (Math::SH::LforN (dirs.rows()))) {
    INFO ("warning: not enough data to estimate spherical harmonic components up to order " + str (lmax));
    lmax = Math::SH::LforN (dirs.rows());
  }
  INFO ("calculating even spherical harmonic components up to order " + str (lmax));

  Math::SH::Transform<float> SHT (dirs, lmax);

  bool normalise = get_options ("normalise").size();
  if (normalise && !bzeros.size())
    throw Exception ("the normalise option is only available if the input data contains b=0 images.");

  header.dim (3) = Math::SH::NforL (lmax);
  header.datatype() = DataType::Float32;
  opt = get_options ("stride");
  if (opt.size()) {
    std::vector<int> strides = opt[0][0];
    if (strides.size() > header.ndim())
      throw Exception ("too many axes supplied to -stride option");
    for (size_t n = 0; n < strides.size(); ++n)
      header.stride(n) = strides[n];
  } else {
    header.stride(0) = 2;
    header.stride(1) = 3;
    header.stride(2) = 4;
    header.stride(3) = 1;
  }
  Image::Buffer<float> SH_data (argument[1], header);

  Image::BufferPreload<float>::voxel_type amp_vox (amp_data);
  Image::Buffer<float>::voxel_type SH_vox (SH_data);

  Math::Vector<float> res (lmax);
  Math::Vector<float> sigs (dirs.rows());

  Image::LoopInOrder loop (SH_vox, "mapping amplitudes to SH coefficients...", 0, 3);
  for (loop.start (SH_vox, amp_vox); loop.ok(); loop.next (SH_vox, amp_vox)) {

    double norm = 0.0;
    if (normalise) {
      for (size_t n = 0; n < bzeros.size(); n++) {
        amp_vox[3] = bzeros[n];
        norm += amp_vox.value ();
      }
      norm /= bzeros.size();
    }

    for (size_t n = 0; n < dirs.rows(); n++) {
      if (dwis.size())
        amp_vox[3] = dwis[n];
      else
        amp_vox[3] = n;
      sigs[n] = amp_vox.value();
      if (normalise) sigs[n] /= norm;
    }

    SHT.A2SH (res, sigs);

    for (SH_vox[3] = 0; SH_vox[3] < SH_vox.dim (3); ++SH_vox[3])
      SH_vox.value() = res[SH_vox[3]];
  }

}
