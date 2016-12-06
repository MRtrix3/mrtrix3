/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "command.h"
#include "progressbar.h"
#include "image.h"
#include "math/SH.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "algo/threaded_loop.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  DESCRIPTION
  + "convert a set of amplitudes (defined along a set of corresponding directions) "
    "to their spherical harmonic representation. The spherical harmonic decomposition is "
    "calculated by least-squares linear fitting."

  + "The directions can be defined either as a DW gradient scheme (for example to compute "
    "the SH representation of the DW signal) or a set of [az el] pairs as output by the dirgen "
    "command. The DW gradient scheme or direction set can be supplied within the input "
    "image header or using the -gradient or -directions option. Note that if a direction set "
    "and DW gradient scheme can be found, the direction set will be used by default."

  + Math::SH::encoding_description;

  ARGUMENTS
  + Argument ("amp", "the input amplitude image.").type_image_in ()
  + Argument ("SH", "the output spherical harmonics coefficients image.").type_image_out ();


  OPTIONS
  + Option ("lmax",
            "set the maximum harmonic order for the output series. By default, the "
            "program will use the highest possible lmax given the number of "
            "diffusion-weighted images, up to a maximum of 8.")
  +   Argument ("order").type_integer (0, 30)

  + Option ("normalise", "normalise the DW signal to the b=0 image")

  + Option ("directions", "the directions corresponding to the input amplitude image used to sample AFD. "
                          "By default this option is not required providing the direction set is supplied "
                          "in the amplitude image. This should be supplied as a list of directions [az el], "
                          "as generated using the dirgen command")
  +   Argument ("file").type_file_in()

  + Option ("rician", "correct for Rician noise induced bias, using noise map supplied")
  +   Argument ("noise").type_image_in()

  + DWI::GradImportOptions()
  + DWI::ShellOption
  + Stride::Options;
}



#define RICIAN_POWER 2.25


typedef float value_type;

class Amp2SHCommon {
  public:
    template <class MatrixType>
      Amp2SHCommon (const MatrixType& sh2amp,
          const std::vector<size_t>& bzeros, 
          const std::vector<size_t>& dwis, 
          bool normalise_to_bzero) :
        sh2amp (sh2amp), 
        amp2sh (Math::pinv (sh2amp)),
        bzeros (bzeros),
        dwis (dwis),
        normalise (normalise_to_bzero) { }


    Eigen::MatrixXd sh2amp, amp2sh;
    const std::vector<size_t>& bzeros;
    const std::vector<size_t>& dwis;
    bool normalise;
};




class Amp2SH {
  public:
    Amp2SH (const Amp2SHCommon& common) : 
      C (common), 
      a (common.amp2sh.cols()),
      s (common.amp2sh.rows()),
      c (common.amp2sh.rows()) { }

    template <class SHImageType, class AmpImageType>
      void operator() (SHImageType& SH, AmpImageType& amp) 
      {
        get_amps (amp);
        c.noalias() = C.amp2sh * a;
        write_SH (SH);
      }



    // Rician-corrected version:
    template <class SHImageType, class AmpImageType, class NoiseImageType>
      void operator() (SHImageType& SH, AmpImageType& amp, const NoiseImageType& noise) 
      {
        w = Eigen::VectorXd::Ones (C.sh2amp.rows());

        get_amps (amp);
        c = C.amp2sh * a;

        for (size_t iter = 0; iter < 20; ++iter) {
          sh2amp = C.sh2amp;
          if (get_rician_bias (sh2amp, noise.value()))
            break;
          for (ssize_t n = 0; n < sh2amp.rows(); ++n) 
            sh2amp.row (n).array() *= w[n];

          s.noalias() = sh2amp.transpose() * ap;
          Q.triangularView<Eigen::Lower>() = sh2amp.transpose() * sh2amp;
          llt.compute (Q);
          c = llt.solve (s);
        }

        write_SH (SH);
      }

  protected:
    const Amp2SHCommon& C;
    Eigen::VectorXd a, s, c, w, ap;
    Eigen::MatrixXd Q, sh2amp;
    Eigen::LLT<Eigen::MatrixXd> llt;

    template <class AmpImageType>
      void get_amps (AmpImageType& amp) {
        double norm = 1.0;
        if (C.normalise) {
          for (size_t n = 0; n < C.bzeros.size(); n++) {
            amp.index(3) = C.bzeros[n];
            norm += amp.value ();
          }
          norm = C.bzeros.size() / norm;
        }

        for (ssize_t n = 0; n < a.size(); n++) {
          amp.index(3) = C.dwis.size() ? C.dwis[n] : n;
          a[n] = amp.value() * norm;
        }
      }

    template <class SHImageType>
      void write_SH (SHImageType& SH) {
        for (auto l = Loop(3) (SH); l; ++l)
          SH.value() = c[SH.index(3)];
      }

    bool get_rician_bias (const Eigen::MatrixXd& sh2amp, default_type noise) {
      ap = sh2amp * c;
      default_type norm_diff = 0.0;
      default_type norm_amp = 0.0;
      for (ssize_t n = 0; n < ap.size() ; ++n) {
        ap[n] = std::max (ap[n], default_type(0.0));
        default_type t = std::pow (ap[n]/noise, default_type(RICIAN_POWER));
        w[n] = Math::pow2 ((t + 1.7)/(t + 1.12));
        default_type diff = a[n] - noise * std::pow (t + 1.65, 1.0/RICIAN_POWER);
        norm_diff += Math::pow2 (diff);
        norm_amp += Math::pow2 (a[n]);
        ap[n] += diff;
      }
      return norm_diff/norm_amp < 1.0e-8; 
    }
};





void run ()
{
  auto amp = Image<value_type>::open (argument[0]).with_direct_io (3);
  Header header (amp);

  std::vector<size_t> bzeros, dwis;
  Eigen::MatrixXd dirs;
  auto opt = get_options ("directions");
  if (opt.size()) {
    dirs = load_matrix (opt[0][0]);
  } 
  else {
    auto hit = header.keyval().find ("directions");
    if (hit != header.keyval().end()) {
      std::vector<default_type> dir_vector;
      for (auto line : split_lines (hit->second)) {
        auto v = parse_floats (line);
        dir_vector.insert (dir_vector.end(), v.begin(), v.end());
      }
      dirs.resize(dir_vector.size() / 2, 2);
      for (size_t i = 0; i < dir_vector.size(); i += 2) {
        dirs(i/2, 0) = dir_vector[i];
        dirs(i/2, 1) = dir_vector[i+1];
      }
    } 
    else {
      auto grad = DWI::get_valid_DW_scheme (amp);
      DWI::Shells shells (grad);
      shells.select_shells (true, true);
      if (shells.smallest().is_bzero())
        bzeros = shells.smallest().get_volumes();
      dwis = shells.largest().get_volumes();
      dirs = DWI::gen_direction_matrix (grad, dwis);
    }
  }

  auto sh2amp = DWI::compute_SH2amp_mapping (dirs, true, 8);


  bool normalise = get_options ("normalise").size();
  if (normalise && !bzeros.size())
    throw Exception ("the normalise option is only available if the input data contains b=0 images.");


  header.size (3) = sh2amp.cols();
  Stride::set_from_command_line (header);
  auto SH = Image<value_type>::create (argument[1], header);

  Amp2SHCommon common (sh2amp, bzeros, dwis, normalise);

  opt = get_options ("rician");
  if (opt.size()) {
    auto noise = Image<value_type>::open (opt[0][0]).with_direct_io();
    ThreadedLoop ("mapping amplitudes to SH coefficients", amp, 0, 3)
      .run (Amp2SH (common), SH, amp, noise);
  }
  else {
    ThreadedLoop ("mapping amplitudes to SH coefficients", amp, 0, 3)
      .run (Amp2SH (common), SH, amp);
  }
}
