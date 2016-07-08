
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


#include "image.h"
#include "sparse/image.h"
#include "sparse/fixel_metric.h"
#include "image_helpers.h"
#include "algo/threaded_loop.h"
using MR::Sparse::FixelMetric;

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "compare two fixel images for differences, within specified tolerance.";

  ARGUMENTS
  + Argument ("data1", "a fixel image.").type_image_in ()
  + Argument ("data2", "another fixel image.").type_image_in ()
  + Argument ("tolerance", "the amount of signal difference to consider acceptable").type_float (0.0);
}


void run ()
{
  Sparse::Image<FixelMetric> buffer1 (argument[0]);
  Sparse::Image<FixelMetric> buffer2 (argument[1]);
  check_dimensions (buffer1, buffer2);
  for (size_t i = 0; i < buffer1.ndim(); ++i) {
    if (std::isfinite (buffer1.spacing(i)))
      if (buffer1.spacing(i) != buffer2.spacing(i))
        throw Exception ("images \"" + buffer1.name() + "\" and \"" + buffer2.name() + "\" do not have matching voxel spacings " +
                                       str(buffer1.spacing(i)) + " vs " + str(buffer2.spacing(i)));
  }
  for (size_t i  = 0; i < 3; ++i) {
    for (size_t j  = 0; j < 4; ++j) {
      if (std::abs (buffer1.transform()(i,j) - buffer2.transform()(i,j)) > 0.001)
        throw Exception ("images \"" + buffer1.name() + "\" and \"" + buffer2.name() + "\" do not have matching header transforms "
                         + "\n" + str(buffer1.transform().matrix()) + "vs \n " + str(buffer2.transform().matrix()) + ")");
    }
  }

  double tol = argument[2];

  ThreadedLoop (buffer1)
    .run ([&tol] (decltype(buffer1)& a, decltype(buffer2)& b) {
       if (a.value().size() != b.value().size())
         throw Exception ("the fixel images do not have corresponding fixels in all voxels");
       // For each fixel
       for (size_t fixel = 0; fixel != a.value().size(); ++fixel) {
         // Check value
         if (std::abs (a.value()[fixel].value - b.value()[fixel].value) > tol)
           throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match fixel value within specified precision of " + str(tol)
               + " (" + str(a.value()[fixel].value) + " vs " + str(b.value()[fixel].value) + ")");
         // Check size
         if (std::abs (a.value()[fixel].size - b.value()[fixel].size) > tol)
           throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match fixel size within specified precision of " + str(tol)
               + " (" + str(a.value()[fixel].size) + " vs " + str(b.value()[fixel].size) + ")");
         // Check Direction
         for (size_t dim = 0; dim < 3; ++dim) {
           if (std::abs (a.value()[fixel].dir[dim] - b.value()[fixel].dir[dim]) > tol)
             throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match fixel direction within specified precision of " + str(tol)
                 + " (" + str(a.value()[fixel].dir[dim]) + " vs " + str(b.value()[fixel].dir[dim]) + ")");
         }
       }
     }, buffer1, buffer2);


  CONSOLE ("data checked OK");
}

