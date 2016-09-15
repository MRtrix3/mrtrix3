
/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2015.

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

#include <fstream>

#include "command.h"

#include <Eigen/Dense>
#include "math/math.h"
 
using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "compare two matrices for differences, optionally with a specified tolerance.";

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
      if (std::abs ((*(in1.data()+i) - *(in2.data()+i)) / (0.5 * (*(in1.data()+i) + *(in2.data()+i)))) > tol)
        throw Exception ("matrices \"" + Path::basename (argument[0]) + "\" and \"" + Path::basename (argument[1]) + " do not match within fractional precision of " + str(tol)
                         + " (" + str(*(in1.data()+i)) + " vs " + str(*(in2.data()+i)) + ")");
    }
  
  } else {
  
    double tol = 0.0;
    opt = get_options ("abs");
    if (opt.size())
      tol = opt[0][0];

    for (size_t i = 0; i != numel; ++i) {
      if (std::abs (*(in1.data()+i) - *(in2.data()+i)) > tol)
        throw Exception ("matrices \"" + Path::basename (argument[0]) + "\" and \"" + Path::basename (argument[1]) + " do not match within absolute precision of " + str(tol)
                         + " (" + str(*(in1.data()+i)) + " vs " + str(*(in2.data()+i)) + ")");
    }
        
  }
  
  CONSOLE ("data checked OK");
}

