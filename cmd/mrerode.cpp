/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08, David Raffelt, 08/06/2012

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

#include "command.h"
#include "image/header.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/filter/erode.h"


using namespace std;
using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (d.tournier@brain.org.au)";

  DESCRIPTION
  + "erode a mask (i.e. binary) image";

  ARGUMENTS
  + Argument ("input", "input mask image to be eroded.").type_image_in ()
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + Option ("npass", "the number of passes (default: 1).")
  + Argument ("number", "the number of passes.").type_integer (1, 1, 1000);
}

void run() {

  Image::Buffer<bool> input_data (argument[0]);
  Image::Buffer<bool>::voxel_type input_voxel (input_data);

  Image::Filter::Erode erode_filter (input_voxel);

  Image::Header output_header (input_data);
  output_header.info() = erode_filter.info();
  Image::Buffer<bool> output_data (argument[1], output_header);
  Image::Buffer<bool>::voxel_type output_voxel (output_data);

  Options opt = get_options ("npass");
  erode_filter.set_npass(static_cast<unsigned int> (opt.size() ? opt[0][0] : 1));

  erode_filter (input_voxel, output_voxel);
}
