/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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
                "be sampled, generated using the gendir command").type_file ()
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

  if (sh_data.ndim() != 4)
    throw Exception ("the input spherical harmonic image should contain 4 dimensions");

  Image::Header amp_header (sh_data);

  Math::Matrix<value_type> directions;

  if (get_options("gradient").size()) {
    Math::Matrix<value_type> grad;
    grad.load (argument[1]);
    std::vector<int> bzeros, dwis;
    DWI::guess_DW_directions (dwis, bzeros, grad);
    DWI::gen_direction_matrix (directions, grad, dwis);
  } 
  else {
    directions.load (argument[1]);
    Math::Matrix<value_type> grad (directions.rows(), 4);
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
      Image::Buffer<value_type>::voxel_type (sh_data), 
      Image::Buffer<value_type>::voxel_type (amp_data),
      Image::NoOp<value_type>, remove_negative (get_options("nonnegative").size()), 3);

}
