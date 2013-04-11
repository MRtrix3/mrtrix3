/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 10/08/2012.

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
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/filter/resize.h"
#include "progressbar.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", NULL };

void usage ()
{
  DESCRIPTION
  + "Resize an image by defining the new image resolution, voxel size or a scale factor."
  + "Note that if the image is 4D, then only the first 3 dimensions can be resized."
  + "Also note that if the image is down-sampled, the appropriate smoothing is automatically applied using Gaussian smoothing.";

  ARGUMENTS
  + Argument ("input", "input image to be smoothed.").type_image_in ()
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + Option ("size", "define the new image size for the output image. "
            "This should be specified as a comma-separated list.")
  + Argument ("dims").type_sequence_int()

  + Option ("voxel", "define the new voxel size for the output image. "
            "This can be specified either as a single value to be used for all dimensions, "
            "or as a comma-separated list of the size for each voxel dimension.")
  + Argument ("size").type_sequence_float()

  + Option ("scale", "scale the image resolution by the supplied factor. "
            "This can be specified either as a single value to be used for all dimensions, "
            "or as a comma-separated list of scale factors for each dimension.")
  + Argument ("factor").type_sequence_float()

  + Option ("interp",
            "set the interpolation method to use when resizing (choices: nearest, linear, cubic, sinc. Default: cubic).")
  + Argument ("method").type_choice (interp_choices)

  + DataType::options();
}


void run () {

  Image::Buffer<float> input_data (argument[0]);
  Image::Buffer<float>::voxel_type input_vox (input_data);

  Image::Filter::Resize resize_filter (input_vox);

  std::vector<float> scale;
  Options opt = get_options ("scale");
  if (opt.size()) {
    scale = parse_floats (opt[0][0]);
    if (scale.size() == 1)
      scale.resize(3, scale[0]);
    resize_filter.set_scale_factor(scale);
  }

  std::vector<float> voxel_size;
  opt = get_options ("voxel");
  if (opt.size()) {
    voxel_size = parse_floats (opt[0][0]);
    if (voxel_size.size() == 1)
      voxel_size.resize (3, voxel_size[0]);
    resize_filter.set_voxel_size(voxel_size);
  }

  std::vector<int> image_size;
  opt = get_options ("size");
  if (opt.size()) {
    image_size = parse_ints(opt[0][0]);
    resize_filter.set_size (image_size);
  }

  int interp = 2;
  opt = get_options ("interp");
  if (opt.size()) {
    interp = opt[0][0];
    resize_filter.set_interp_type (interp);
  }

  if ( ((scale.size() > 0) + (voxel_size.size() > 0) + (image_size.size() > 0)) == 0)
    throw Exception ("please use either the -scale, -voxel, or -resolution option to resize the image");
  if ( ((scale.size() > 0) + (voxel_size.size() > 0) + (image_size.size() > 0)) != 1)
    throw Exception ("only a single method can be used to resize the image (image resolution, voxel size or scale factor)");

  Image::Header header (input_data);
  header.info() = resize_filter.info();
  header.datatype() = DataType::from_command_line (header.datatype());
  Image::Buffer<float> output_data (argument[1], header);
  Image::Buffer<float>::voxel_type output_vox (output_data);

  resize_filter (input_vox, output_vox);
}
