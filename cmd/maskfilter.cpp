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
#include "image/filter/dilate.h"
#include "image/filter/erode.h"
#include "image/filter/lcc.h"
#include "image/filter/median.h"


using namespace MR;
using namespace App;



const char* filters[] = { "dilate", "erode", "lcc", "median", NULL };




const OptionGroup DilateErodeOption = OptionGroup ("Options for dilate / erode filters")

  + Option ("npass", "the number of times to repeatedly apply the filter")
    + Argument ("value").type_integer (1, 1, 1e6);



const OptionGroup LCCOption = OptionGroup ("Options for largest connected-component filter")

  + Option ("neighbour26", "use 26 adjacent voxels for determining connectivity rather than 6");



const OptionGroup MedianOption = OptionGroup ("Options for median filter")

  + Option ("extent", "specify the extent (width) of kernel size in voxels. "
            "This can be specified either as a single value to be used for all axes, "
            "or as a comma-separated list of the extent for each axis. The default is 3x3x3.")
  + Argument ("voxels").type_sequence_int();








Image::Filter::Base* create_dilate_filter (Image::BufferPreload<bool>::voxel_type& input)
{
  Image::Filter::Dilate* filter = new Image::Filter::Dilate (input);
  Options opt = get_options ("npass");
  if (opt.size())
    filter->set_npass (int(opt[0][0]));
  return filter;
}

Image::Filter::Base* create_erode_filter (Image::BufferPreload<bool>::voxel_type& input)
{
  Image::Filter::Erode* filter = new Image::Filter::Erode (input);
  Options opt = get_options ("npass");
  if (opt.size())
    filter->set_npass (int(opt[0][0]));
  return filter;
}



Image::Filter::Base* create_lcc_filter (Image::BufferPreload<bool>::voxel_type& input)
{
  Image::Filter::LargestConnectedComponent* filter = new Image::Filter::LargestConnectedComponent (input);
  filter->set_large_neighbourhood (get_options ("neighbour26").size());
  return filter;
}



Image::Filter::Base* create_median_filter (Image::BufferPreload<bool>::voxel_type& input)
{
  Image::Filter::Median* filter = new Image::Filter::Median (input);
  Options opt = get_options ("extent");
  if (opt.size())
    filter->set_extent (parse_ints (opt[0][0]));
  return filter;
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
  auto input_voxel = input_data.voxel();

  const size_t filter_index = argument[1];

  Image::Filter::Base* filter = NULL;
  switch (filter_index) {
    case 0: filter = create_dilate_filter (input_voxel); break;
    case 1: filter = create_erode_filter  (input_voxel); break;
    case 2: filter = create_lcc_filter    (input_voxel); break;
    case 3: filter = create_median_filter (input_voxel); break;
    default: assert (0);
  }

  Image::Header header;
  header.info() = filter->info();
  Image::Stride::set_from_command_line (header);

  Image::Buffer<bool> output_data (argument[2], header);
  auto output_voxel = output_data.voxel();

  filter->set_message (std::string("applying ") + std::string(argument[1]) + " filter to image " + std::string(argument[0]) + "... ");

  switch (filter_index) {
    case 0: (*dynamic_cast<Image::Filter::Dilate*>                    (filter)) (input_voxel, output_voxel); break;
    case 1: (*dynamic_cast<Image::Filter::Erode*>                     (filter)) (input_voxel, output_voxel); break;
    case 2: (*dynamic_cast<Image::Filter::LargestConnectedComponent*> (filter)) (input_voxel, output_voxel); break;
    case 3: (*dynamic_cast<Image::Filter::Median*>                    (filter)) (input_voxel, output_voxel); break;
  }

  delete filter;

}
