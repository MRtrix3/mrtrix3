/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "command.h"
#include "image.h"
#include "phase_encoding.h"
#include "progressbar.h"
#include "algo/threaded_loop.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "math/SH.h"

#include "dwi/directions/directions.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Convert a set of amplitudes (defined along a set of corresponding directions) "
    "to their spherical harmonic representation";

  DESCRIPTION
  + "The spherical harmonic decomposition is calculated by least-squares linear fitting "
    "to the amplitude data."

  + DWI::Directions::directions_description

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

  + DWI::Directions::directions_option ("associating input image amplitudes with directions to facilitate SH transformation", "Read from the input image header contents")

  + Option ("rician", "correct for Rician noise induced bias, using noise map supplied")
  +   Argument ("noise").type_image_in()

  + DWI::GradImportOptions()
  + DWI::ShellsOption
  + Stride::Options;
}



#define RICIAN_POWER 2.25


using value_type = float;

class Amp2SHCommon {
  public:
    template <class MatrixType>
      Amp2SHCommon (const MatrixType& sh2amp,
          const vector<size_t>& bzeros,
          const vector<size_t>& dwis,
          bool normalise_to_bzero) :
        sh2amp (sh2amp),
        amp2sh (Math::pinv (sh2amp)),
        bzeros (bzeros),
        dwis (dwis),
        normalise (normalise_to_bzero) { }


    Eigen::MatrixXd sh2amp, amp2sh;
    const vector<size_t>& bzeros;
    const vector<size_t>& dwis;
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

  vector<size_t> bzeros, dwis;
  Eigen::MatrixXd dirs;

  auto opt = get_options ("directions");
  if (opt.size()) {
    if (get_options ("grad").size() || get_options ("fslgrad").size())
      throw Exception ("Cannot use -directions in conjunction with -grad or -fslgrad; only one source of a direction set can be provided");
    dirs = DWI::Directions::load (std::string (opt[0][0]), true);
    if (dirs.cols() == 4)
      throw Exception ("If importing a gradient table from an external file, use -grad command-line option rather than -directions");
    if (dirs.rows() != header.size (3))
      throw Exception ("Number of directions as imported via -directions option (" + str(dirs.rows()) + ") does not match number of input image volumes (" + str(header.size(3)) + ")");
    dirs = Math::Sphere::to_spherical (dirs);
    dwis.reserve (dirs.rows());
    for (size_t i = 0; i != dirs.rows(); ++i)
      dwis.push_back (i);
  } else {
    // Try loading the full gradient table first;
    //   only revert to checking for "directions" within the input image header if we can't do so
    try {
      auto grad = DWI::get_DW_scheme (amp);
      DWI::Shells shells (grad);
      shells.select_shells (true, false, false);
      if (shells.smallest().is_bzero())
        bzeros = shells.smallest().get_volumes();
      dwis = shells.largest().get_volumes();
      dirs = DWI::gen_direction_matrix (grad, dwis);
      DWI::stash_DW_scheme (header, grad);
    } catch (Exception&) {
      try {
        auto directions_it = header.keyval().find ("directions");
        if (directions_it != header.keyval().end()) {
          dirs = deserialise_matrix (directions_it->second);
          Math::Sphere::check (dirs, header.size (3));
          header.keyval()["prior_directions"] = directions_it->second;
          header.keyval().erase (directions_it);
        }
      } catch (Exception&) {
        throw Exception ("Unable to locate any basis direction set with which to convert input amplitudes to SH");
      }
    }
  }

  PhaseEncoding::clear_scheme (header);

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
