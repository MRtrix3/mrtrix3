/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include <fstream>

#include "command.h"
#include "types.h"

#include <Eigen/Dense>
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

  const default_type tolerance_frac = get_option_value ("frac", 0.0);
  const default_type tolerance_abs = get_option_value ("abs", 0.0);

  Eigen::MatrixXd in1, in2;

  try {
    in1 = load_matrix<double> (argument[0]);
    in2 = load_matrix<double> (argument[1]);
  } catch (Exception&) {

    const auto in1c = load_matrix<cdouble> (argument[0]);
    const auto in2c = load_matrix<cdouble> (argument[1]);

    if (in1c.rows() != in2c.rows() || in1c.cols() != in2c.cols())
      throw Exception ("matrices \"" + Path::basename (argument[0]) + "\" and \"" + Path::basename (argument[1]) + "\" do not have matching sizes"
                       " (" + str(in1.rows()) + "x" + str(in1c.cols()) + " vs " + str(in2c.rows()) + "x" + str(in2c.cols()) + ")");

    if (bool(tolerance_frac)) {
      for (ssize_t col = 0; col != in1c.cols(); ++col) {
        for (ssize_t row = 0; row != in1c.rows(); ++row) {
          if ((abs (in1c(row, col).real() - in2c(row, col).real()) / (0.5 * (in1c(row, col).real() + in2c(row, col).real())) > tolerance_frac)
              || (abs (in1c(row, col).imag() - in2c(row, col).imag()) / (0.5 * (in1c(row, col).imag() + in2c(row, col).imag())) > tolerance_frac))
            throw Exception ("matrices \"" + Path::basename (argument[0]) + "\" and \"" + Path::basename (argument[1]) + "\" do not match within fractional precision of " + str(tolerance_frac)
                             + " ((" + str(row) + ", " + str(col) + "): " + str(in1c(row, col)) + " vs " + str(in2c(row, col)) + ")");
        }
      }
    }

    if (bool(tolerance_abs) || !bool(tolerance_frac)) {
      for (ssize_t col = 0; col != in1c.cols(); ++col) {
        for (ssize_t row = 0; row != in1c.rows(); ++row) {
          if ((abs (in1c(row, col).real() - in2c(row, col).real()) > tolerance_abs)
              || (abs (in1c(row, col).imag() - in2c(row, col).imag()) > tolerance_abs))
            throw Exception ("matrices \"" + Path::basename (argument[0]) + "\" and \"" + Path::basename (argument[1]) + "\" do not match within absolute precision of " + str(tolerance_abs)
                             + " ((" + str(row) + ", " + str(col) + "): " + str(in1c(row, col)) + " vs " + str(in2c(row, col)) + ")");
        }
      }
    }

    return;
  }

  if (in1.rows() != in2.rows() || in1.cols() != in2.cols())
    throw Exception ("matrices \"" + Path::basename (argument[0]) + "\" and \"" + Path::basename (argument[1]) + "\" do not have matching sizes"
                     " (" + str(in1.rows()) + "x" + str(in1.cols()) + " vs " + str(in2.rows()) + "x" + str(in2.cols()) + ")");

  if (bool(tolerance_frac)) {
    for (ssize_t col = 0; col != in1.cols(); ++col) {
      for (ssize_t row = 0; row != in1.rows(); ++row) {
        if (abs (in1(row, col) - in2(row, col)) / (0.5 * (in1(row, col) + in2(row, col))) > tolerance_frac)
          throw Exception ("matrices \"" + Path::basename (argument[0]) + "\" and \"" + Path::basename (argument[1]) + "\" do not match within fractional precision of " + str(tolerance_abs)
                           + " ((" + str(row) + ", " + str(col) + "): " + str(in1(row, col)) + " vs " + str(in2(row, col)) + ")");
      }
    }
  }

  if (bool(tolerance_abs) || !bool(tolerance_frac)) {
    for (ssize_t col = 0; col != in1.cols(); ++col) {
      for (ssize_t row = 0; row != in1.rows(); ++row) {
        if (abs (in1(row, col) - in2(row, col)) > tolerance_abs)
          throw Exception ("matrices \"" + Path::basename (argument[0]) + "\" and \"" + Path::basename (argument[1]) + "\" do not match within absolute precision of " + str(tolerance_abs)
                           + " ((" + str(row) + ", " + str(col) + "): " + str(in1(row, col)) + " vs " + str(in2(row, col)) + ")");
      }
    }
  }

  CONSOLE ("data checked OK");
}

