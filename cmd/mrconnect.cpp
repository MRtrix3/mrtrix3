/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 29 May 2012

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
#include "image/buffer_preload.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/filter/connected_components.h"
#include "math/matrix.h"


using namespace MR;
using namespace App;
using namespace std;


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Connected component labelling of a binary input image. Each connected "
    "component is labelled with a unique integer in order of component size."
  + "Note that if the input image is 4D then the default behaviour is to connect components "
    "within each 3D volume (see the -axes option to change this behaviour). "  ;

  ARGUMENTS
  + Argument ("image_in",
              "the binary image to be labelled").type_image_in()
  + Argument ("image_out",
              "the labelled output image").type_image_out();

  OPTIONS
  + Option ("axes",
            "specify which axes should be included in the connected components. By default only "
            "the first 3 axes are included. The axes should be provided as a comma-separated list of values.")
  + Argument ("axes").type_sequence_int()

  + Option ("largest",
            "only retain the largest component")

  + Option ("connectivity",
            "use 26 neighbourhood connectivity (Default: 6)");
}


void run ()
{
  Image::Header input_header (argument[0]);
  Image::Buffer<bool> input_data (input_header);
  auto input_voxel = input_data.voxel();

  Image::Filter::ConnectedComponents connected_filter(input_voxel);
  Image::Header header (input_data);
  header.info() = connected_filter.info();
  Image::Buffer<int> output_data (argument[1], header);
  auto output_vox = output_data.voxel();

  Options opt = get_options ("axes");
  std::vector<int> axes;
  if (opt.size()) {
    axes = opt[0][0];
    for (size_t d = 0; d < input_data.ndim(); d++)
      connected_filter.set_ignore_dim (d, true);
    for (size_t i = 0; i < axes.size(); i++) {
      if (axes[i] >= static_cast<int> (input_header.ndim()) || axes[i] < 0)
        throw Exception ("axis supplied to option -ignore is out of bounds");
      connected_filter.set_ignore_dim (axes[i], false);
    }
  }

  opt = get_options ("largest");
  if (opt.size())
    connected_filter.set_largest_only (true);

  opt = get_options ("connectivity");
  if (opt.size())
    connected_filter.set_26_connectivity(true);

  connected_filter.set_message ("computing connected components...");
  connected_filter (input_voxel, output_vox);
}

