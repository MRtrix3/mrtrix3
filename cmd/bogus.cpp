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
#include "math/complex.h"
#include "math/vector.h"
#include "math/matrix.h"

using namespace MR;

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "this is used to test stuff.",
  NULL
};

ARGUMENTS = {
  Argument ("x", "input vector"),
  Argument ("M", "input matrix"),
  Argument() 
};

OPTIONS = { Option() };

typedef cfloat T;

EXECUTE {

  Math::Vector<T> x;
  x.load (std::string (argument[0]));
  VAR (x);

  Math::Matrix<T> M;
  M.load (std::string (argument[1]));
  VAR (M);

  Math::Vector<T> y;

  Math::mult (y, M, x);
  VAR (y);
}

