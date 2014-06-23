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
  +   Argument ("order").type_integer (0, 8, 30)

  + Option ("normalise", "normalise the DW signal to the b=0 image")

  + Option ("directions", "the directions corresponding to the input amplitude image used to sample AFD. "
                          "By default this option is not required providing the direction set is supplied "
                          "in the amplitude image. This should be supplied as a list of directions [az el], "
                          "as generated using the gendir command")
  +   Argument ("file").type_file()

  + Option ("rician", "correct for Rician noise induced bias, using noise map supplied")
  +   Argument ("noise").type_image_in()

  + DWI::GradOption
  + DWI::ShellOption
  + Image::Stride::StrideOption;
}



#define RICIAN_POWER 2.25


typedef float value_type;

class Amp2SHCommon {
  public:
    Amp2SHCommon (const Math::Matrix<value_type>& dirs, size_t lmax, 
        const std::vector<size_t>& bzeros, const std::vector<size_t>& dwis, bool normalise_to_bzero) :
      SHT (dirs, lmax),
      bzeros (bzeros),
      dwis (dwis),
      normalise (normalise_to_bzero) { }

  Math::SH::Transform<value_type> SHT;
  const std::vector<size_t>& bzeros;
  const std::vector<size_t>& dwis;
  bool normalise;
};




class Amp2SH {
  public:
    Amp2SH (const Amp2SHCommon& common) : 
      C (common), 
      a (C.SHT.n_amp()), 
      c (C.SHT.n_SH()) { }

    template <class SHVoxelType, class AmpVoxelType>
      void operator() (SHVoxelType& SH, AmpVoxelType& amp) 
      {
        get_amps (amp);
        C.SHT.A2SH (c, a);
        write_SH (SH);
      }



    // Rician-corrected version:
    template <class SHVoxelType, class AmpVoxelType, class NoiseVoxelType>
      void operator() (SHVoxelType& SH, AmpVoxelType& amp, const NoiseVoxelType& noise) 
      {
        const Math::Matrix<value_type>& M (C.SHT.mat_SH2A());
        w.allocate (M.rows());
        w = value_type(1.0);

        get_amps (amp);
        C.SHT.A2SH (c, a);

        for (size_t iter = 0; iter < 20; ++iter) {
          MW = M;
          if (get_rician_bias (M, noise.value()))
            break;
          for (size_t n = 0; n < MW.rows(); ++n) 
            MW.row (n) *= w[n];

          Math::mult (c, value_type(1.0), CblasTrans, MW, ap);
          Math::mult (Q, value_type(1.0), CblasTrans, MW, CblasNoTrans, M);
          Math::Cholesky::decomp (Q);
          Math::Cholesky::solve (c, Q);
        }

        write_SH (SH);
      }

  protected:
    const Amp2SHCommon& C;
    Math::Vector<value_type> a, c, w, ap;
    Math::Matrix<value_type> Q, MW;

    template <class AmpVoxelType>
      void get_amps (AmpVoxelType& amp) {
        double norm = 1.0;
        if (C.normalise) {
          for (size_t n = 0; n < C.bzeros.size(); n++) {
            amp[3] = C.bzeros[n];
            norm += amp.value ();
          }
          norm = C.bzeros.size() / norm;
        }

        for (size_t n = 0; n < C.SHT.n_amp(); n++) {
          amp[3] = C.dwis.size() ? C.dwis[n] : n;
          a[n] = amp.value() * norm;
        }
      }

    template <class SHVoxelType>
      void write_SH (SHVoxelType& SH) {
        for (SH[3] = 0; SH[3] < SH.dim (3); ++SH[3])
          SH.value() = c[SH[3]];
      }

    bool get_rician_bias (const Math::Matrix<value_type>& M, value_type noise) {
      Math::mult (ap, M, c);
      value_type norm_diff = 0.0;
      value_type norm_amp = 0.0;
      for (size_t n = 0; n < ap.size() ; ++n) {
        ap[n] = std::max (ap[n], value_type(0.0));
        value_type t = Math::pow (ap[n]/noise, value_type(RICIAN_POWER));
        w[n] = 1.0;//Math::pow2 ((t + 1.7)/(t + 1.12));
        value_type diff = a[n] - noise * Math::pow (t + 1.65, 1.0/RICIAN_POWER);
        norm_diff += Math::pow2 (diff);
        norm_amp += Math::pow2 (a[n]);
        ap[n] += diff;
      }
      return norm_diff/norm_amp < 1.0e-8; 
    }
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
  } else {
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
    } else {
      Math::Matrix<value_type> grad = DWI::get_valid_DW_scheme<value_type> (amp_data);
      DWI::Shells shells (grad);
      shells.select_shells (true, true);
      if (shells.smallest().is_bzero())
        bzeros = shells.smallest().get_volumes();
      dwis = shells.largest().get_volumes();
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


  bool normalise = get_options ("normalise").size();
  if (normalise && !bzeros.size())
    throw Exception ("the normalise option is only available if the input data contains b=0 images.");


  header.dim (3) = Math::SH::NforL (lmax);
  header.datatype() = DataType::Float32;
  Image::Stride::set_from_command_line (header);
  Image::Buffer<value_type> SH_data (argument[1], header);

  Image::BufferPreload<value_type>::voxel_type amp_vox (amp_data);
  Image::Buffer<value_type>::voxel_type SH_vox (SH_data);

  Amp2SHCommon common (dirs, lmax, bzeros, dwis, normalise);

  opt = get_options ("rician");
  if (opt.size()) {
    Image::BufferPreload<value_type> noise_data (opt[0][0]);
    Image::BufferPreload<value_type>::voxel_type noise_vox (noise_data);
    Image::ThreadedLoop ("mapping amplitudes to SH coefficients...", amp_vox, 1, 0, 3)
      .run (Amp2SH (common), SH_vox, amp_vox, noise_vox);
  }
  else {
    Image::ThreadedLoop ("mapping amplitudes to SH coefficients...", amp_vox, 1, 0, 3)
      .run (Amp2SH (common), SH_vox, amp_vox);
  }
}
