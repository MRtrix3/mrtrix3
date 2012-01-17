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
#include "debug.h"
#include "image/data_preload.h"
#include "image/scratch.h"
#include "image/voxel.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;


App::OptionGroup special_options () 
{
  using namespace App;
  OptionGroup specific_options = OptionGroup ("My options")
    + Option ("specific", "some description")
    +  Argument ("arg")

    + Option ("special", "more text")
    +   Argument ("x").type_image_in()
    +   Argument ("y").type_file();

  return specific_options;
}



void usage () {

  AUTHOR = "Joe Bloggs";

  VERSION[0] = 1;
  VERSION[1] = 4;
  VERSION[2] = 3;

  COPYRIGHT = "whatever you want";

  DESCRIPTION 
    + "this is used to test stuff. I need to write a lot of stuff here to pad this out and check that the wrapping functionality works as advertised... Seems to do an OK job so far. Wadaya reckon?"
    + "some more details here.";

  ARGUMENTS
    + Argument ("input", "the input image.").type_image_in ();

  OPTIONS
    + Option ("poo", "its description")
    + Argument ("arg1").type_integer (0,1,10)
    + Argument ("arg2")

    + Option ("crap", "another description")
    + Argument ("stuff").type_float (-1.0, 0.0, 1.0)

    + special_options();
}





void run () 
{

  Image::Header header_in (argument[0]);

  Image::Stride::List stride (header_in.ndim(), 0);
  stride[2] = 1;

  Image::DataPreload<float> data (header_in, stride);

  VAR (header_in);
  VAR (data);

  Image::DataPreload<float>::voxel_type vox (data);
  VAR (vox);

  stride[3] = 1;
  stride[0] = stride[1] = stride[2] = 0;
  Image::Scratch<uint8_t> scratch (header_in, stride, "my scratch buffer");
  VAR (scratch);

  Image::Scratch<uint8_t>::voxel_type scratch_vox (scratch);
  VAR (scratch_vox);
  
  std::cout << "values: [ ";
  for (size_t n = 0; n < Image::voxel_count (data); n += 10000)
    std::cout << data.get(n) << " ";
  std::cout << "]\n";

}

