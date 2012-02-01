/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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
#include "image/data.h"
#include "image/voxel.h"
#include "filter/median3D.h"

MRTRIX_APPLICATION

using namespace MR; 
using namespace App; 

void usage () {
  DESCRIPTION
    + "smooth images using median filtering.";

  ARGUMENTS 
    + Argument ("input", "input image to be median-filtered.").type_image_in ()
    + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
    + Option ("extent", "specify extent of median filtering neighbourhood in voxels. "
        "This can be specified either as a single value to be used for all 3 axes, "
        "or as a comma-separated list of 3 values, one for each axis (default: 3x3x3).")
    + Argument ("size").type_sequence_int();
}



void run () {
  std::vector<int> extent (1);
  extent[0] = 3;
  
  Options opt = get_options ("extent");

  if (opt.size())
    extent = parse_ints (opt[0][0]);

  Image::Data<float> src_data (argument[0]);
  Image::Data<float>::voxel_type src (src_data);

  Filter::Median3DFilter<Image::Data<float>::voxel_type, Image::Data<float>::voxel_type > median_filter(src);
  median_filter.set_extent(extent);

  Image::Data<float> dest_data (src_data, argument[1]);
  Image::Data<float>::voxel_type dest (dest_data);

  median_filter.execute (dest);
}

