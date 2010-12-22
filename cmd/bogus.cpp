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
#include "image/voxel.h"
#include "math/rng.h"
#include "math/matrix.h"

using namespace MR;

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "this is used to test stuff.",
  NULL
};

ARGUMENTS = { Argument() };

OPTIONS = { Option() };





typedef float T;

EXECUTE {

  using namespace Math;
  Vector<T> V (10);
  VAR (V);
  V = 0.0;
  VAR (V);
  V.sub (3,5) = 1.0;
  VAR (V);
  V.sub (4,8) += 2.0;
  VAR (V);

  {
    Vector<T> U = V.sub (5,10);
    U += 5.0;
    VAR (U);
  }
  VAR (V);

  V.sub (2,5) += V.sub (6,9);
  VAR (V);
}

