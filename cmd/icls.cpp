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
#include "timer.h"
#include "debug.h"

#define MRTRIX_ICLS_DEBUG
#include "math/constrained_least_squares.h"


using namespace MR;
using namespace App;




void usage ()
{

  DESCRIPTION
    + "test ICLS";

  ARGUMENTS
    + Argument ("problem", "the problem matrix.").type_file_in()
    + Argument ("constraint", "the constraint matrix.").type_file_in()
    + Argument ("b", "the RHS vector.").type_file_in();
}






void run ()
{
  auto H = load_matrix<double> (argument[0]);
  auto A = load_matrix<double> (argument[1]);
  auto b = load_vector<double> (argument[2]);

  Math::ICLS::Problem<double> icls_problem (H, A, 0.0, 1e-10);//, 1.0e-10, 1.0e-6);
  Math::ICLS::Solver<double> icls_solver (icls_problem);

  Eigen::VectorXd x;
  Timer timer;
  size_t niter = icls_solver (x, b);
  VAR (timer.elapsed());
  if (niter > icls_problem.max_niter) 
    WARN ("failed to converge");
  Eigen::VectorXd c = A * x;
  VAR (c.minCoeff());
  VAR (niter);
  std::cout << x.transpose() << std::endl;
}

