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
#include "math/rng.h"
#include "math/matrix.h"


using namespace MR;
using namespace App;




void usage ()
{

  DESCRIPTION
    + "generate random data to test ICLS";

  ARGUMENTS
    + Argument ("num_meas", "the nummber of measurements to fit to.").type_integer()
    + Argument ("num_param", "the number of parameters in the model.").type_integer()
    + Argument ("num_constraints", "the number of constraints.").type_integer();
}






void run ()
{
  const size_t nmeas = argument[0];
  const size_t nparam = argument[1];
  const size_t nconst = argument[2];

  Math::RNG rng;

  {
    Math::Matrix<double> H (nmeas, nparam);
    for (size_t i = 0; i < H.rows(); ++i)
      for (size_t j = 0; j < H.columns(); ++j)
        H(i,j) = rng.normal();

    for (size_t i = 0; i < H.rows(); ++i)
      H.row(i) *= std::exp(-20.0) * std::exp (30.0 * rng.uniform()) / Math::pow2 (nmeas);
    H.save ("H.txt", 16);
  }




  {
    Math::Matrix<double> A (nconst, nparam);
    for (size_t i = 0; i < A.rows(); ++i)
      for (size_t j = 0; j < A.columns(); ++j)
        A(i,j) = rng.normal();

    for (size_t i = 0; i < A.rows(); ++i) {
      if (A(i,0) < 0.0)
        A.row(i) *= -1.0;
    }
    A.save ("A.txt", 16);
  }



  {
    Math::Vector<double> b (nmeas);
    for (size_t i = 0; i < b.size(); ++i)
      b[i] = rng.normal();
    b.save ("b.txt", 16);
  }

}


