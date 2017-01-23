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

#include "command.h"
#include "progressbar.h"
#include "datatype.h"
#include "math/rng.h"

#include "image.h"
#include "algo/threaded_loop.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  DESCRIPTION
  + "generate a test image of random numbers";

  ARGUMENTS
  + Argument ("size", "the dimensions of the test data.").type_sequence_int ()
  + Argument ("data", "the output image.").type_image_out ();

  OPTIONS
    + Stride::Options
    + DataType::options();
}


void run ()
{
  vector<int> dim = argument[0];

  Header header;

  header.ndim() = dim.size();
  for (size_t n = 0; n < dim.size(); ++n) {
    header.size(n) = dim[n];
    header.spacing(n) = 1.0f;
  }
  header.datatype() = DataType::from_command_line (DataType::Float32);
  Stride::set_from_command_line (header, Stride::contiguous_along_spatial_axes (header));

  auto image = Header::create (argument[1], header).get_image<float>();

  struct fill { NOMEMALIGN
    Math::RNG rng;
    std::normal_distribution<float> normal;
    void operator() (decltype(image)& v) { v.value() = normal(rng); }
  };
  ThreadedLoop ("generating random data...", image).run (fill(), image);
}

