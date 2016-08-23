
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
#include "algo/threaded_loop.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com) and David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "compare two images for differences, optionally with a specified tolerance.";

  ARGUMENTS
  + Argument ("data1", "an image.").type_image_in()
  + Argument ("data2", "another image.").type_image_in();
  
  OPTIONS
  + Option ("abs", "specify an absolute tolerance") 
    + Argument ("tolerance").type_float (0.0)
  + Option ("frac", "specify a fractional tolerance") 
    + Argument ("tolerance").type_float (0.0);
}


void run ()
{
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

  auto opt = get_options ("frac");
  if (opt.size()) {
  
    const double tol = opt[0][0];
    
    ThreadedLoop (in1)
    .run ([&tol] (const decltype(in1)& a, const decltype(in2)& b) {
        if (std::abs ((a.value() - b.value()) / (0.5 * (a.value() + b.value()))) > tol)
        throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match within fractional precision of " + str(tol)
             + " (" + str(cdouble (a.value())) + " vs " + str(cdouble (b.value())) + ")");
        }, in1, in2);
  
  } else {
  
    double tol = 0.0;
    opt = get_options ("abs");
    if (opt.size())
      tol = opt[0][0];

    ThreadedLoop (in1)
    .run ([&tol] (const decltype(in1)& a, const decltype(in2)& b) {
        if (std::abs (a.value() - b.value()) > tol)
        throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match within absolute precision of " + str(tol)
             + " (" + str(cdouble (a.value())) + " vs " + str(cdouble (b.value())) + ")");
        }, in1, in2);
        
  }

  CONSOLE ("data checked OK");
}

