/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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
 * See the Mozila Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include <sstream>

#include "command.h"
#include "math/SH.h"
#include "image.h"
#include "dwi/gradient.h"


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Evaluate the amplitude of an image of spherical harmonic functions along specified directions";

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
    + Option ("gradient",
              "assume input directions are supplied as a gradient encoding file")

    + Option ("nonnegative",
              "cap all negative amplitudes to zero")

    + Stride::Options
    + DataType::options();
}


using value_type = float;


class SH2Amp { MEMALIGN(SH2Amp)
  public:
    template <class MatrixType>
    SH2Amp (const MatrixType& dirs, const size_t lmax, bool nonneg) :
      transformer (dirs.template cast<value_type>(), lmax),
      nonnegative (nonneg),
      sh (transformer.n_SH()),
      amp (transformer.n_amp()) { }

    void operator() (Image<value_type>& in, Image<value_type>& out) {
      sh = in.row (3);
      transformer.SH2A(amp, sh);
      if (nonnegative)
        amp = amp.cwiseMax(value_type(0.0));
      out.row (3) = amp;
    }

  private:
    const Math::SH::Transform<value_type> transformer;
    const bool nonnegative;
    Eigen::Matrix<value_type, Eigen::Dynamic, 1> sh, amp;
};



void run ()
{
  auto sh_data = Image<value_type>::open(argument[0]);
  Math::SH::check (sh_data);

  Header amp_header (sh_data);

  Eigen::MatrixXd directions;

  if (get_options("gradient").size()) {
    Eigen::MatrixXd grad;
    grad = load_matrix(argument[1]);
    DWI::Shells shells (grad);
    directions = DWI::gen_direction_matrix (grad, shells.largest().get_volumes());
  } else {
    directions = load_matrix(argument[1]);
  }

  if (!directions.rows())
    throw Exception ("no directions found in input directions file");

  std::stringstream dir_stream;
  for (ssize_t d = 0; d < directions.rows() - 1; ++d)
    dir_stream << directions(d,0) << "," << directions(d,1) << "\n";
  dir_stream << directions(directions.rows() - 1,0) << "," << directions(directions.rows() - 1,1);
  amp_header.keyval().insert(std::pair<std::string, std::string> ("directions", dir_stream.str()));

  amp_header.size(3) = directions.rows();
  Stride::set_from_command_line (amp_header, Stride::contiguous_along_axis (3, amp_header));
  amp_header.datatype() = DataType::from_command_line (DataType::Float32);

  auto amp_data = Image<value_type>::create(argument[2], amp_header);

  SH2Amp sh2amp (directions, Math::SH::LforN (sh_data.size(3)), get_options("nonnegative").size());
  ThreadedLoop("computing amplitudes", sh_data, 0, 3, 2).run (sh2amp, sh_data, amp_data);

}
