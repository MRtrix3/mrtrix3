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
#include "image.h"
#include "filter/resize.h"
#include "progressbar.h"


using namespace MR;
using namespace App;

const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", NULL };

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Resize an image by defining the new image resolution, voxel size or a scale factor."
  + "Note that if the image is 4D, then only the first 3 dimensions can be resized."
  + "Also note that if the image is down-sampled, the appropriate smoothing is automatically applied using Gaussian smoothing.";

  ARGUMENTS
  + Argument ("input", "input image to be resized.").type_image_in ()
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


  auto input = Image<float>::open (argument[0]);

  Filter::Resize resize_filter (input);

  size_t resize_option_count = 0;

  std::vector<default_type> scale;
  auto opt = get_options ("scale");
  if (opt.size()) {
    scale = parse_floats (opt[0][0]);
    if (scale.size() == 1)
      scale.resize (3, scale[0]);
    resize_filter.set_scale_factor (scale);
    ++resize_option_count;
  }

  std::vector<default_type> voxel_size;
  opt = get_options ("voxel");
  if (opt.size()) {
    voxel_size = parse_floats (opt[0][0]);
    if (voxel_size.size() == 1)
      voxel_size.resize (3, voxel_size[0]);
    resize_filter.set_voxel_size (voxel_size);
    ++resize_option_count;
  }

  std::vector<int> image_size;
  opt = get_options ("size");
  if (opt.size()) {
    image_size = parse_ints(opt[0][0]);
    resize_filter.set_size (image_size);
    ++resize_option_count;
  }


  int interp = 2;
  opt = get_options ("interp");
  if (opt.size()) {
    interp = opt[0][0];
    resize_filter.set_interp_type (interp);
  }

  if (!resize_option_count)
    throw Exception ("please use either the -scale, -voxel, or -resolution option to resize the image");
  if (resize_option_count != 1)
    throw Exception ("only a single method can be used to resize the image (image resolution, voxel size or scale factor)");

  Header header (resize_filter);
  header.datatype() = DataType::from_command_line (DataType::from<float> ());
  auto output = Image<float>::create (argument[1], header);

  resize_filter (input, output);
}
