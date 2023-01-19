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

#include <sstream>

#include "command.h"
#include "image.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "math/sphere.h"
#include "math/SH.h"

#include "dwi/directions/file.h"


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Evaluate the amplitude of an image of spherical harmonic functions along specified directions";

  DESCRIPTION
    + "The input image should consist of a 4D or 5D image, with SH coefficients "
      "along the 4th dimension according to the convention below. If 4D (or "
      "size 1 along the 5th dimension), the program expects to be provided with "
      "a single shell of directions. If 5D, each set of coefficients along the "
      "5th dimension is understood to correspond to a different shell."
    + "The directions can be provided as:\n"
      "- a 2-column ASCII text file contained azimuth / elevation pairs (as "
      "produced by dirgen)\n"
      "- a 3-column ASCII text file containing x, y, z Cartesian direction "
      "vectors (as produced by dirgen -cart)\n"
      "- a 4-column ASCII text file containing the x, y, z, b components of a "
      "full DW encoding scheme (in MRtrix format, see main documentation for "
      "details).\n"
      "- an image file whose header contains a valid DW encoding scheme"
    + "If a full DW encoding is provided, the number of shells needs to match "
      "those found in the input image of coefficients (i.e. its size along the 5th "
      "dimension). If needed, the -shell option can be used to pick out the "
      "specific shell(s) of interest."
    + "If the input image contains multiple shells (its size along the 5th "
      "dimension is greater than one), the program will expect the direction "
      "set to contain multiple shells, which can only be provided as a full DW "
      "encodings (the last two options in the list above)."
    + Math::SH::encoding_description;

  ARGUMENTS
    + Argument ("input",
                "the input image consisting of spherical harmonic (SH) "
                "coefficients.").type_image_in ()
    + Argument ("directions",
                "the list of directions along which the SH functions will "
                "be sampled, generated using the dirgen command").type_file_in ()
    + Argument ("output",
                "the output image consisting of the amplitude of the SH "
                "functions along the specified directions.").type_image_out ();

  OPTIONS
    + Option ("nonnegative",
              "cap all negative amplitudes to zero")
    + DWI::GradImportOptions()
    + Stride::Options
    + DataType::options();
}


using value_type = float;





class SH2Amp { MEMALIGN(SH2Amp)
  public:
    SH2Amp (const Eigen::MatrixXd& transform, bool nonneg) :
      transform (transform),
      nonnegative (nonneg) { }

    void operator() (Image<value_type>& in, Image<value_type>& out) {
      sh = in.row (3);
      amp = transform * sh;
      if (nonnegative)
        amp = amp.cwiseMax(0.0);
      out.row (3) = amp;
    }

  private:
    const Eigen::MatrixXd& transform;
    const bool nonnegative;
    Eigen::VectorXd sh, amp;
};






class SH2AmpMultiShell { MEMALIGN(SH2AmpMultiShell)
  public:
    SH2AmpMultiShell (const vector<Eigen::MatrixXd>& dirs, const DWI::Shells& shells, bool nonneg) :
      transforms (dirs),
      shells (shells),
      nonnegative (nonneg) { }

    void operator() (Image<value_type>& in, Image<value_type>& out) {
      for (size_t n = 0; n < transforms.size(); ++n) {
        if (in.ndim() > 4)
          in.index(4) = n;
        sh = in.row (3);

        amp = transforms[n] * sh;

        if (nonnegative)
          amp = amp.cwiseMax(value_type(0.0));

        for (ssize_t k = 0; k < amp.size(); ++k) {
          out.index(3) = shells[n].get_volumes()[k];
          out.value() = amp[k];
        }
      }
    }

  private:
    const vector<Eigen::MatrixXd>& transforms;
    const DWI::Shells& shells;
    const bool nonnegative;
    Eigen::VectorXd sh, amp;
};






void run ()
{
  auto sh_data = Image<value_type>::open(argument[0]);
  Math::SH::check (sh_data);
  const size_t lmax = Math::SH::LforN (sh_data.size(3));

  Eigen::MatrixXd directions;
  try {
    directions = DWI::Directions::load_spherical (argument[1]);
  }
  catch (Exception& E) {
    try {
      directions = load_matrix<double> (argument[1]);
      if (directions.cols() < 4)
        throw ("unable to interpret file \"" + std::string(argument[1]) + "\" as a directions or gradient file");
    }
    catch (Exception& E) {
      auto header = Header::open (argument[1]);
      directions = DWI::get_DW_scheme (header);
    }
  }

  if (!directions.size())
    throw Exception ("no directions found in input directions file");

  Header amp_header (sh_data);
  amp_header.ndim() = 4;
  amp_header.size(3) = directions.rows();
  Stride::set_from_command_line (amp_header, Stride::contiguous_along_axis (3, amp_header));
  amp_header.datatype() = DataType::from_command_line (DataType::Float32);

  if (directions.cols() == 2) { // single-shell:

    if (sh_data.ndim() > 4 && sh_data.size(4) > 1) {
      Exception excp ("multi-shell input data provided with single-shell direction set");
      excp.push_back ("  use full DW scheme to operate on multi-shell data");
      throw excp;
    }

    amp_header.ndim() = 4;

    std::stringstream dir_stream;
    for (ssize_t d = 0; d < directions.rows() - 1; ++d)
      dir_stream << directions(d,0) << "," << directions(d,1) << "\n";
    dir_stream << directions(directions.rows() - 1,0) << "," << directions(directions.rows() - 1,1);
    amp_header.keyval()["directions"] = dir_stream.str();

    auto amp_data = Image<value_type>::create(argument[2], amp_header);
    auto transform = Math::SH::init_transform (directions, lmax);

    SH2Amp sh2amp (transform, get_options("nonnegative").size());
    ThreadedLoop("computing amplitudes", sh_data, 0, 3, 2).run (sh2amp, sh_data, amp_data);

  }
  else { // full gradient scheme:

    DWI::set_DW_scheme (amp_header, directions);
    auto shells = DWI::Shells (directions).select_shells (false, false, false);

    if (shells.count() == 0)
      throw Exception ("no shells found in gradient scheme");

    if (shells.count() > 1) {
      if (sh_data.ndim() < 5)
        throw Exception ("multiple shells detected in gradient scheme, but only one shell in input data");
      if (sh_data.size(4) != ssize_t (shells.count()))
        throw Exception ("number of shells differs between gradient scheme and input data");
    }
    else if (! (sh_data.ndim() == 4 || ( sh_data.ndim() > 4 && ( sh_data.size(4) != 1 ))) )
      throw Exception ("number of shells differs between gradient scheme and input data");

    vector<Eigen::MatrixXd> transforms;

    for (size_t n = 0; n < shells.count(); ++n) {
      Eigen::MatrixXd dirs (shells[n].count(), 2);
      if (shells[n].is_bzero()) {
        dirs.setConstant (0.0);
      }
      else {
        for (size_t idx = 0; idx < shells[n].count(); ++idx)
          Math::Sphere::cartesian2spherical (directions.row (shells[n].get_volumes()[idx]).head (3), dirs.row (idx));
      }
      transforms.push_back (Math::SH::init_transform (dirs, lmax));
    }

    auto amp_data = Image<value_type>::create(argument[2], amp_header);

    SH2AmpMultiShell sh2amp (transforms, shells, get_options("nonnegative").size());
    ThreadedLoop("computing amplitudes", sh_data, 0, 3).run (sh2amp, sh_data, amp_data);

  }
}

