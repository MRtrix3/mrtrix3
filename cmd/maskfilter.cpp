/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 28/03/2014.

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
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/voxel.h"
#include "image/filter/base.h"
#include "image/filter/connected_components.h"
#include "image/filter/dilate.h"
#include "image/filter/erode.h"
#include "image/filter/median.h"


using namespace MR;
using namespace App;



const char* filters[] = { "dilate", "erode", "lcc", "median", NULL };




const OptionGroup DilateErodeOption = OptionGroup ("Options for dilate / erode filters")

  + Option ("npass", "the number of times to repeatedly apply the filter")
    + Argument ("value").type_integer (1, 1, 1e6);



const OptionGroup LCCOption = OptionGroup ("Options for connected-components filter")

  + Option ("neighbour26", "use 26 adjacent voxels for determining connectivity rather than 6");



const OptionGroup MedianOption = OptionGroup ("Options for median filter")

  + Option ("extent", "specify the extent (width) of kernel size in voxels. "
            "This can be specified either as a single value to be used for all axes, "
            "or as a comma-separated list of the extent for each axis. The default is 3x3x3.")
  + Argument ("voxels").type_sequence_int();








void parse_dilate_filter_cmdline_options (Image::Filter::Dilate& filter)
{
  Options opt = get_options ("npass");
  if (opt.size())
    filter.set_npass (int(opt[0][0]));
}

void parse_erode_filter_cmdline_options (Image::Filter::Erode& filter)
{
  Options opt = get_options ("npass");
  if (opt.size())
    filter.set_npass (int(opt[0][0]));
}



void parse_lcc_filter_cmdline_options (Image::Filter::ConnectedComponents& filter)
{
  filter.set_largest_only (true);
  filter.set_26_connectivity (get_options ("neighbour26").size());
}



void parse_median_filter_cmdline_options (Image::Filter::Median& filter)
{
  Options opt = get_options ("extent");
  if (opt.size())
    filter.set_extent (parse_ints (opt[0][0]));
}





void usage ()
{
  AUTHOR = "Robert E. Smith (r.smith@brain.org.au), David Raffelt (d.raffelt@brain.org.au) and J-Donald Tournier (jdtournier@gmail.com)";

  DESCRIPTION
  + "Perform filtering operations on 3D / 4D mask images."

  + "The available filters are: dilate, erode, lcc, median."

  + "Each filter has its own unique set of optional parameters.";


  ARGUMENTS
  + Argument ("input",  "the input image.").type_image_in ()
  + Argument ("filter", "the type of filter to be applied").type_choice (filters)
  + Argument ("output", "the output image.").type_image_out ();


  OPTIONS
  + DilateErodeOption
  + LCCOption
  + MedianOption

  + Image::Stride::StrideOption;

}


void run () {

  Image::BufferPreload<bool> input_data (argument[0]);
  Image::BufferPreload<bool>::voxel_type input_voxel (input_data);

  const size_t filter_index = argument[1];

  Image::Filter::Base* filter = NULL;
  switch (filter_index) {
    case 0:
      filter = new Image::Filter::Dilate (input_voxel);
      parse_dilate_filter_cmdline_options (*(dynamic_cast<Image::Filter::Dilate*> (filter)));
      break;
    case 1:
      filter = new Image::Filter::Erode (input_voxel);
      parse_erode_filter_cmdline_options (*(dynamic_cast<Image::Filter::Erode*> (filter)));
      break;
    case 2:
      filter = new Image::Filter::ConnectedComponents (input_voxel);
      parse_lcc_filter_cmdline_options (*(dynamic_cast<Image::Filter::ConnectedComponents*> (filter)));
      break;
    case 3:
      filter = new Image::Filter::Median (input_voxel);
      parse_median_filter_cmdline_options (*(dynamic_cast<Image::Filter::Median* > (filter)));
      break;
    default:
      assert (0);
  }

  Image::Header header;
  header.info() = filter->info();

  Options opt = get_options ("stride");
  if (opt.size()) {
    std::vector<int> strides = opt[0][0];
    if (strides.size() > input_data.ndim())
      throw Exception ("too many axes supplied to -stride option");
    for (size_t n = 0; n < strides.size(); ++n)
      header.stride(n) = strides[n];
  }

  Image::Buffer<bool> output_data (argument[2], header);
  Image::Buffer<bool>::voxel_type output_voxel (output_data);

  filter->set_message (std::string("applying ") + std::string(argument[1]) + " filter to image " + std::string(argument[0]));

  switch (filter_index) {
    case 0: (*dynamic_cast<Image::Filter::Dilate*>              (filter)) (input_voxel, output_voxel); break;
    case 1: (*dynamic_cast<Image::Filter::Erode* >              (filter)) (input_voxel, output_voxel); break;
    case 2: (*dynamic_cast<Image::Filter::ConnectedComponents*> (filter)) (input_voxel, output_voxel); break;
    case 3: (*dynamic_cast<Image::Filter::Median* >             (filter)) (input_voxel, output_voxel); break;
  }

  delete filter;

}
