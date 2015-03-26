/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 06/07/11.

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

#include <sstream>

#include "command.h"
#include "math/SH.h"
#include "image/matrix_multiply.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "dwi/gradient.h"


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Evaluate the amplitude of an image of spherical harmonic functions "
    "along the specified directions";

  ARGUMENTS
    + Argument ("input",
                "the input image consisting of spherical harmonic (SH) "
                "coefficients.").type_image_in ()
    + Argument ("directions",
                "the list of directions along which the SH functions will "
                "be sampled, generated using the gendir command").type_file_in ()
    + Argument ("output",
                "the output image consisting of the amplitude of the SH "
                "functions along the specified directions.").type_image_out ();

  OPTIONS
    + Option ("gradient",
              "assume input directions are supplied as a gradient encoding file")

    + Option ("nonnegative",
              "cap all negative amplitudes to zero")

    + Image::Stride::StrideOption
    + DataType::options();
}


typedef float value_type;

class remove_negative 
{
  public:
    remove_negative (bool nonnegative) : nonnegative (nonnegative) { }
    value_type operator () (value_type val) const { return nonnegative ? std::max (val, value_type (0.0)) : val; }
  protected:
    const bool nonnegative;
};



void run ()
{
  Image::Buffer<value_type> sh_data (argument[0]);
  Math::SH::check (sh_data);

  Image::Header amp_header (sh_data);

  Math::Matrix<value_type> directions;

  if (get_options("gradient").size()) {
    Math::Matrix<value_type> grad;
    grad.load (argument[1]);
    DWI::Shells shells (grad);
    directions = DWI::gen_direction_matrix (grad, shells.largest().get_volumes());
  } else {
    directions.load (argument[1]);
  }

  if (!directions.rows())
    throw Exception ("no directions found in input directions file");

  std::stringstream dir_stream;
  for (size_t d = 0; d < directions.rows() - 1; ++d)
    dir_stream << directions(d,0) << "," << directions(d,1) << "\n";
  dir_stream << directions(directions.rows() - 1,0) << "," << directions(directions.rows() - 1,1);
  amp_header.insert(std::pair<std::string, std::string> ("directions", dir_stream.str()));

  amp_header.dim(3) = directions.rows();
  Image::Stride::set_from_command_line (amp_header, Image::Stride::contiguous_along_axis (3));
  amp_header.datatype() = DataType::from_command_line (DataType::Float32);

  Image::Buffer<value_type> amp_data (argument[2], amp_header);

  Math::SH::Transform<value_type> transformer (directions, Math::SH::LforN (sh_data.dim(3)));
  Image::matrix_multiply (
      "computing amplitudes...",
      transformer.mat_SH2A(),
      sh_data.voxel(), 
      amp_data.voxel(),
      Image::NoOp<value_type>, remove_negative (get_options("nonnegative").size()), 3);

}
