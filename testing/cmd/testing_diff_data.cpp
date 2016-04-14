
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

#ifdef MRTRIX_UPDATED_API
 
# include "image.h"
# include "algo/threaded_loop.h"
 
#else
 
# include "image/buffer.h"
# include "image/voxel.h"
# include "image/threaded_loop.h"
 
#endif


using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "compare two images for differences, within specified tolerance.";

  ARGUMENTS
  + Argument ("data1", "an image.").type_image_in ()
  + Argument ("data2", "another image.").type_image_in ()
  + Argument ("tolerance", "the amount of signal difference to consider acceptable").type_float (0.0);
}


void run ()
{
#ifdef MRTRIX_UPDATED_API

  auto in1 = Image<cdouble>::open (argument[0]);
  auto in2 = Image<cdouble>::open (argument[1]);
  check_dimensions (in1, in2);
  for (size_t i = 0; i < in1.ndim(); ++i) {
    if (std::isfinite (in1.size(i)))
      if (in1.size(i) != in2.size(i))
        throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" do not have matching voxel spacings " +
                                       str(in1.size(i)) + " vs " + str(in2.size(i)));
  }
  for (size_t i  = 0; i < 3; ++i) {
    for (size_t j  = 0; j < 4; ++j) {
      if (std::abs (in1.transform().matrix()(i,j) - in2.transform().matrix()(i,j)) > 0.001)
        throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" do not have matching header transforms "
                           + "\n" + str(in1.transform().matrix()) + "vs \n " + str(in2.transform().matrix()) + ")");
    }
  }


  double tol = argument[2];

  ThreadedLoop (in1)
    .run ([&tol] (const decltype(in1)& a, const decltype(in2)& b) {
        if (std::abs (a.value() - b.value()) > tol)
        throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match within specified precision of " + str(tol)
             + " (" + str(cdouble (a.value())) + " vs " + str(cdouble (b.value())) + ")");
        }, in1, in2);

#else

  Image::Buffer<cdouble> buffer1 (argument[0]);
  Image::Buffer<cdouble> buffer2 (argument[1]);
  Image::check_dimensions (buffer1, buffer2);
  for (size_t i = 0; i < buffer1.ndim(); ++i) {
    if (std::isfinite (buffer1.vox(i)))
      if (buffer1.vox(i) != buffer2.vox(i))
        throw Exception ("images \"" + buffer1.name() + "\" and \"" + buffer2.name() + "\" do not have matching voxel spacings " +
                                       str(buffer1.vox(i)) + " vs " + str(buffer2.vox(i)));
  }
  for (size_t i  = 0; i < 4; ++i) {
    for (size_t j  = 0; j < 4; ++j) {
      if (std::abs (buffer1.transform()(i,j) - buffer2.transform()(i,j)) > 0.001)
        throw Exception ("images \"" + buffer1.name() + "\" and \"" + buffer2.name() + "\" do not have matching header transforms "
                         + "\n" + str(buffer1.transform()) + "vs \n " + str(buffer2.transform()) + ")");
    }
  }

  double tol = argument[2];

  Image::ThreadedLoop (buffer1)
    .run ([&tol] (const decltype(buffer1.voxel())& a, const decltype(buffer2.voxel())& b) {
       if (std::abs (a.value() - b.value()) > tol)
         throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match within specified precision of " + str(tol) 
             + " (" + str(cdouble (a.value())) + " vs " + str(cdouble (b.value())) + ")");
     }, buffer1.voxel(), buffer2.voxel());

#endif

  CONSOLE ("data checked OK");
}

