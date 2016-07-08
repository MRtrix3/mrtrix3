
/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt

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

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "compare two images for differences, within specified tolerance.";

  ARGUMENTS
  + Argument ("dir1", "directions file").type_file_in ()
  + Argument ("dir2", "another directions file.").type_file_in ()
  + Argument ("tolerance", "the amount of difference to consider acceptable").type_float (0.0);
}


void run ()
{
  double tol = argument[2];

  Eigen::MatrixXd dir1 = load_matrix (argument[0]);
  Eigen::MatrixXd dir2 = load_matrix (argument[1]);

  if (dir1.cols() != dir2.cols())
    throw Exception ("number of columns is not the same");

  if (dir1.rows() != dir2.rows())
    throw Exception ("number of rows is not the same");

  for (ssize_t i = 0; i < dir1.cols(); ++i) {
    for (ssize_t j = 0; j < dir1.rows(); ++j) {
      if (std::abs (dir1(i,j) - dir2(i,j)) > tol)
        throw Exception ("directions \"" + str(argument[0]) + "\" and \"" + str(argument[1]) + "\" do not match within specified precision of " + str(tol)
            + " (" + str(cdouble (dir1(i,j))) + " vs " + str(cdouble (dir2(i,j))) + ")");
    }
  }

  CONSOLE ("directions checked OK");
}

