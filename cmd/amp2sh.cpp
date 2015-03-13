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
#include "dwi/shells.h"
#include "image/threaded_loop.h"


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
  + Argument ("file", "a list of directions [az el] generated using the gendir command.").type_file_in()

  + DWI::GradImportOptions
  + DWI::ShellOption
  + Image::Stride::StrideOption;
}





typedef float value_type;

class Amp2SHCommon {
  public:
    Amp2SHCommon (const Math::Matrix<value_type>& sh2amp,
        const std::vector<size_t>& bzeros, 
        const std::vector<size_t>& dwis, 
        bool normalise_to_bzero) :
      amp2sh (Math::pinv (sh2amp)),
      bzeros (bzeros),
      dwis (dwis),
      normalise (normalise_to_bzero) { }

  Math::Matrix<value_type> amp2sh;
  const std::vector<size_t>& bzeros;
  const std::vector<size_t>& dwis;
  bool normalise;
};




class Amp2SH {
  public:
    Amp2SH (const Amp2SHCommon& common) : 
      C (common), 
      a (common.amp2sh.columns()),
      c (common.amp2sh.rows())
  { }

    template <class SHVoxelType, class AmpVoxelType>
      void operator() (SHVoxelType& SH, AmpVoxelType& amp) 
      {
        double norm = 1.0;
        if (C.normalise) {
          for (size_t n = 0; n < C.bzeros.size(); n++) {
            amp[3] = C.bzeros[n];
            norm += amp.value ();
          }
          norm = C.bzeros.size() / norm;
        }

        for (size_t n = 0; n < a.size(); n++) {
          amp[3] = C.dwis.size() ? C.dwis[n] : n;
          a[n] = amp.value() * norm;
        }

        mult (c, C.amp2sh, a);

        for (SH[3] = 0; SH[3] < SH.dim (3); ++SH[3])
          SH.value() = c[SH[3]];
      }

  protected:
    const Amp2SHCommon& C;
    Math::Vector<value_type> a, c;
};





void run ()
{
  Image::BufferPreload<value_type> amp_data (argument[0], Image::Stride::contiguous_along_axis (3));
  Image::Header header (amp_data);

  std::vector<size_t> bzeros, dwis;
  Math::Matrix<value_type> dirs;
  Options opt = get_options ("directions");
  if (opt.size()) {
    dirs.load(opt[0][0]);
  }
  else {
    if (header["directions"].size()) {
      std::vector<value_type> dir_vector;
      std::vector<std::string > lines = split (header["directions"], "\n", true);
      for (size_t l = 0; l < lines.size(); l++) {
        std::vector<value_type> v (parse_floats (lines[l]));
        dir_vector.insert (dir_vector.end(), v.begin(), v.end());
      }
      dirs.resize(dir_vector.size() / 2, 2);
      for (size_t i = 0; i < dir_vector.size(); i += 2) {
        dirs(i / 2, 0) = dir_vector[i];
        dirs(i / 2, 1) = dir_vector[i + 1];
      }
    }
    else {
      Math::Matrix<value_type> grad = DWI::get_valid_DW_scheme<value_type> (amp_data);
      DWI::get_gradient_table_and_directions (amp_data, grad, dirs, dwis, bzeros);
    }
  }

  Math::Matrix<value_type> sh2amp = DWI::compute_SH2amp_mapping (dirs, true, 8);

  bool normalise = get_options ("normalise").size();
  if (normalise && !bzeros.size())
    throw Exception ("the normalise option is only available if the input data contains b=0 images.");

  header.dim (3) = sh2amp.columns();
  header.datatype() = DataType::Float32;
  Image::Stride::set_from_command_line (header);
  Image::Buffer<value_type> SH_data (argument[1], header);

  auto amp_vox = amp_data.voxel();
  auto SH_vox = SH_data.voxel();

  Amp2SHCommon common (sh2amp, bzeros, dwis, normalise);
  Image::ThreadedLoop ("mapping amplitudes to SH coefficients...", amp_vox)
    .run (Amp2SH (common), SH_vox, amp_vox);
}
