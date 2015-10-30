/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 2015

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
#include "image.h"
#include "algo/threaded_loop.h"


namespace MR {
}


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "replaces voxels in a deformation field that point to 0,0,0 with nan,nan,nan. "
    "This should be used when computing a MRtrix compatible deformation field by "
    "converting warps generated from any other registration package.";

  ARGUMENTS
  + Argument ("in", "the input warp image.").type_image_in ()
  + Argument ("out", "the output warp image.").type_image_out ();
}


typedef float value_type;


void run ()
{
  auto input = Image<value_type>::open (argument[0]).with_direct_io (Stride::contiguous_along_axis(3));
  if (input.ndim() != 4)
    throw Exception ("input warp is not a 4D image");

  if (input.size(3) != 3)
    throw Exception ("input warp should have 3 volumes in the 4th dimension");

  auto output = Image<value_type>::create (argument[1], input);

  Eigen::Matrix<value_type, 3, 1> nans (std::numeric_limits<value_type>::quiet_NaN(),
                                        std::numeric_limits<value_type>::quiet_NaN(),
                                        std::numeric_limits<value_type>::quiet_NaN());

  auto func = [&](Image<value_type>& in, Image<value_type>& out) {
    if (in.row(3).norm() == 0.0)
      out.row(3) = nans;
    else
      out.row(3) = in.row(3);
  };

  ThreadedLoop ("correcting warp...", input, 0, 3)
    .run (func, input, output);
}
