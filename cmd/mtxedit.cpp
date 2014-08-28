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
#include "image/header.h"
#include "math/matrix.h"


using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "edit transformation matrices."

  + "This is needed in particular to convert the transformation matrix provided "
    "FSL's flirt command to a format usable in MRtrix.";

  ARGUMENTS
  + Argument ("input", "input transformation matrix").type_file_in ()
  + Argument ("from", "the image the input transformation matrix maps from").type_image_in ()
  + Argument ("to", "the image the input transformation matrix maps onto").type_image_in ()
  + Argument ("output", "the output transformation matrix.").type_file_out ();
}



void run ()
{
  Math::Matrix<float> input;
  input.load (argument[0]);

  const Image::Header from (argument[1]);
  const Image::Header to (argument[2]);

  Math::Matrix<float> R(4,4);
  R.identity();
  R(0,0) = -1.0;
  R(0,3) = (to.dim(0)-1) * to.vox(0);
  VAR (R);
  VAR (input);

  Math::Matrix<float> M;
  Math::mult (M, R, input);
  VAR (M);

  R(0,3) = (from.dim(0)-1) * from.vox(0);
  VAR (R);

  Math::mult (input, M, R);
  VAR (input);

  Math::mult (R, to.transform(), input);

  R.save (argument[3]);
}

