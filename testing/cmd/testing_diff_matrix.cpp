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


#include <fstream>

#include "command.h"
#include "math/math.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Compare two matrices for differences, optionally with a specified tolerance";

  ARGUMENTS
  + Argument ("matrix1", "a matrix file.").type_file_in()
  + Argument ("matrix2", "another matrix file.").type_file_in();

  OPTIONS
  + Option ("abs", "specify an absolute tolerance")
    + Argument ("tolerance").type_float (0.0)
  + Option ("frac", "specify a fractional tolerance")
    + Argument ("tolerance").type_float (0.0);
}


void run ()
{

  const Eigen::MatrixXf in1 = load_matrix<float> (argument[0]);
  const Eigen::MatrixXf in2 = load_matrix<float> (argument[1]);

  if (in1.rows() != in2.rows() || in1.cols() != in2.cols())
    throw Exception ("matrices \"" + Path::basename (argument[0]) + "\" and \"" + Path::basename (argument[1]) + "\" do not have matching sizes"
                     " (" + str(in1.rows()) + "x" + str(in1.cols()) + " vs " + str(in2.rows()) + "x" + str(in2.cols()) + ")");

  const size_t numel = in1.size();

  auto opt = get_options ("frac");
  if (opt.size()) {

    const double tol = opt[0][0];

    for (size_t i = 0; i != numel; ++i) {
      if (abs ((*(in1.data()+i) - *(in2.data()+i)) / (0.5 * (*(in1.data()+i) + *(in2.data()+i)))) > tol)
        throw Exception ("matrices \"" + Path::basename (argument[0]) + "\" and \"" + Path::basename (argument[1]) + " do not match within fractional precision of " + str(tol)
                         + " (" + str(*(in1.data()+i)) + " vs " + str(*(in2.data()+i)) + ")");
    }

  } else {

    double tol = 0.0;
    opt = get_options ("abs");
    if (opt.size())
      tol = opt[0][0];

    for (size_t i = 0; i != numel; ++i) {
      if (abs (*(in1.data()+i) - *(in2.data()+i)) > tol)
        throw Exception ("matrices \"" + Path::basename (argument[0]) + "\" and \"" + Path::basename (argument[1]) + " do not match within absolute precision of " + str(tol)
                         + " (" + str(*(in1.data()+i)) + " vs " + str(*(in2.data()+i)) + ")");
    }

  }

  CONSOLE ("data checked OK");
}

