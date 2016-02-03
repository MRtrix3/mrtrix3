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
  auto input = Image<value_type>::open (argument[0]).with_direct_io (3);
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

  ThreadedLoop ("correcting warp", input, 0, 3)
    .run (func, input, output);
}
