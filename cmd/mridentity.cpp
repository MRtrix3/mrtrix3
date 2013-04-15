/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 22 Oct 2011

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
#include "dwi/gradient.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/loop.h"
#include "math/LU.h"
#include "image/transform.h"
#include "image/stride.h"


MRTRIX_APPLICATION

using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Set the image header transformation to the identity (i.e. align the "
    "scanner and image coordinate frames). If the input image is a DWI then "
    "gradient directions are also reoriented (since they are define w.r.t. scanner "
    "coordinates).\n\nThis application is a required pre-processing step to speed up "
    "FOD registration. This application can also be used to reorient gradients for storing "
    "DWIs in the Analyse image format.";

  ARGUMENTS
  + Argument ("input",
              "the input image").type_image_in()
  + Argument ("output",
              "the output image").type_image_out();

  OPTIONS
  + DWI::GradOption

  + Image::Stride::StrideOption;
}


void run ()
{
  Image::Header input_header (argument[0]);

  Math::Matrix<float> grad = DWI::get_DW_scheme<float> (input_header);

  if (grad.is_set()) {
    Image::Transform transform (input_header);
    for (uint dir = 0; dir < grad.rows(); dir++) {
      Math::Vector<float> grad_rotated(3);
      transform.scanner2image_dir(grad.row(dir).sub(0,3), grad_rotated);
      grad.row(dir).sub(0,3) = grad_rotated;
    }
  }

  Math::Matrix<float> identity;
  Image::Transform::set_default (identity, input_header);
  Image::Header output_header (input_header);
  output_header.transform() = identity;
  if (grad.is_set()) {
    output_header.DW_scheme() = grad;
  }

  Options opt = get_options ("stride");
  if (opt.size()) {
    std::vector<int> strides = opt[0][0];
    if (strides.size() > output_header.ndim())
      throw Exception ("too many axes supplied to -stride option");
    for (size_t n = 0; n < strides.size(); ++n)
      output_header.stride(n) = strides[n];
  }

  Image::Buffer<float> output_data (argument[1], output_header);
  Image::Buffer<float>::voxel_type output_voxel(output_data);
  Image::Buffer<float> input_data (input_header);
  Image::Buffer<float>::voxel_type input_voxel(input_data);

  Image::threaded_copy_with_progress_message ("aligning scanner and image coordinate axes...", input_voxel, output_voxel);
}

