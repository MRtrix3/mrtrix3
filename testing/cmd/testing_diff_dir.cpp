/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "command.h"
#include "progressbar.h"
#include "datatype.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Compare two direction sets for differences, within specified tolerance";

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
        throw Exception ("direction files \"" + str(argument[0]) + "\" and \"" + str(argument[1]) + "\" do not match within specified precision of " + str(tol)
            + " (" + str(dir1(i,j)) + " vs " + str(dir2(i,j)) + ")");
    }
  }

  CONSOLE ("directions checked OK");
}

