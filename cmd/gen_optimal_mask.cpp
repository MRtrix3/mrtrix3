/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 03/06/11.

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
#include "point.h"
#include "image/voxel.h"
#include "filter/optimal_threshold.h"
#include "ptr.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (draffelt@gmail.com)";

  DESCRIPTION
  + "Generates an optimal mask based on the parameter free method defined in "
  "Ridgway G et al. (2009) NeuroImage.44(1):99-111.";

  ARGUMENTS
  + Argument ("image",
              "the input image to be masked")
  .type_image_in ()

  + Argument ("image",
              "the output mask image")
  .type_image_out ();
}


typedef float value_type;

void run () {
  Image::Header input_header (argument[0]);
  assert (!input_header.is_complex());
  Image::Voxel<value_type> input_voxel (input_header);

  Image::Header mask_header (input_header);
  mask_header.set_datatype (DataType::Bit);

  Filter::OptimalThreshold<Image::Voxel<float>, Image::Voxel<int> > filter (input_voxel);

  mask_header.set_params (filter.get_output_params());
  mask_header.create (argument[1]);
  Image::Voxel<int> mask_voxel (mask_header);

  filter.execute (mask_voxel);
}

