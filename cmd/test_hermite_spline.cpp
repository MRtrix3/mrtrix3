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
#include "debug.h"
#include "math/hermite.h"
#include "math/vector.h"


using namespace MR;
using namespace App;




void usage ()
{
  DESCRIPTION
    + "test Hermite spline interpolation";

  ARGUMENTS
    + Argument ("control_points", "the control point positions.").type_file()
    + Argument ("values", "the values at the control points.").type_file()
    + Argument ("positions", "the positions to interpolate to.").type_file();
}






void run ()
{
  Math::Vector<float> CP, values, pos;

  CP.load (argument[0]);
  values.load (argument[1]);
  pos.load (argument[2]);

  Math::HermiteSplines<float> spline (CP);
  for (size_t n = 0; n < pos.size(); ++n) {
    spline.set (pos[n]);
    std::cout << pos[n] << " " << spline.value (values) << "\n";
  }
}

