/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 06/04/2010

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
#include "image/voxel.h"
#include "image/buffer.h"
#include "image/interp/nearest.h"
#include "image/interp/linear.h"
#include "image/interp/cubic.h"
#include "image/interp/sinc.h"
#include "image/filter/reslice.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", NULL };

void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Resample an image to a different resolution by a given sample factor.";

ARGUMENTS
  + Argument ("input", "the input image.").type_image_in ()
  + Argument ("factor", "the sample factor").type_float (0.01, 2.0, 100.0)
  + Argument ("output", "the output image .").type_image_out ();

OPTIONS
  + Option ("interp", "set the interpolation method when resampling (default: cubic).")
  + Argument ("method", "the interpolation method.").type_choice (interp_choices);
}


typedef float value_type;

void run () {
  Image::Buffer<value_type> input_data (argument[0]);
  assert (!input_data.datatype().is_complex());

  const float sample_factor = argument[1];

  // TODO handle resample factors that are not neat multiples of 1
  Image::Header output_header (input_data);
  for (int n = 0; n < 3; n++){
    output_header.dim(n) = input_data.dim(n) * sample_factor;
    output_header.vox(n) = input_data.vox(n) / sample_factor;
  }
  output_header.datatype() = DataType::Float32;

  Image::Buffer<float>::voxel_type in  (input_data);

  Image::Buffer<float> data_out (argument[2], output_header);
  Image::Buffer<float>::voxel_type out (data_out);

  int interp = 2;
  Options opt = get_options ("interp");
  if (opt.size()) interp = opt[0][0];

  Math::Matrix<float> T(4,4);
  T.identity();

  switch (interp) {
    case 0: Image::Filter::reslice<Image::Interp::Nearest> (out, in, T); break;
    case 1: Image::Filter::reslice<Image::Interp::Linear>  (out, in, T); break;
    case 2: Image::Filter::reslice<Image::Interp::Cubic>   (out, in, T); break;
    case 3: Image::Filter::reslice<Image::Interp::Sinc>    (out, in, T); break;
    default: assert (0);
  }
}
