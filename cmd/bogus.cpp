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
#include "math/rng.h"
#include "math/least_squares.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;



void usage () {

  DESCRIPTION 
    + "this is used to test stuff. I need to write a lot of stuff here to pad this out and check that the wrapping functionality works as advertised... Seems to do an OK job so far. Wadaya reckon?"
    + "some more details here.";

  ARGUMENTS
    + Argument ("mask", "mask").type_image_in()
    + Argument ("in", "in").type_image_in()
    + Argument ("out", "out").type_image_out();
}


typedef float value_type;


void run () 
{
  Math::RNG rng;

  Math::Matrix<value_type> M (6,9);
  VAR (M.rows());
  VAR (M.columns());
  for (size_t i = 0; i < M.rows(); ++i)
    for (size_t j = 0; j < M.columns(); ++j)
      M(i,j) = rng.normal();

  M.save ("M.txt");
  Math::pinv (M).save("iM.txt");
}

