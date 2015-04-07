
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
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/threaded_loop.h"


using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "compare two images for differences, within specified tolerance.";

  ARGUMENTS
  + Argument ("data1", "the output image.").type_image_in ()
  + Argument ("data2", "the output image.").type_image_in ()
  + Argument ("tolerance", "the amount of signal difference to consider acceptable").type_float (0.0, 0.0);
}


void run ()
{
  Image::Buffer<cdouble> buffer1 (argument[0]);
  Image::Buffer<cdouble> buffer2 (argument[1]);
  Image::check_dimensions (buffer1, buffer2);
  double tol = argument[2];

  Image::ThreadedLoop (buffer1)
    .run ([&tol] (const decltype(buffer1.voxel())& a, const decltype(buffer2.voxel())& b) {
       if (std::abs (a.value() - b.value()) > tol)
         throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match within specified precision of " + str(tol));
     }, buffer1.voxel(), buffer2.voxel());
  CONSOLE ("data checked OK");
}

