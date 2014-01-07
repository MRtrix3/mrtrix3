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
#include "math/vector.h"
#include "math/matrix.h"


using namespace MR;
using namespace App;



void usage () {

  ARGUMENTS
    + Argument ("file", "a file name").type_file();
}


typedef float value_type;


void run () 
{
  Math::RNG rng;


  Math::Vector<value_type> V_orig (1000);
  for (size_t i = 0; i < V_orig.size(); ++i)
      V_orig[i] = rng.normal();
  V_orig.save (argument[0]);

  Math::Vector<value_type> V (static_cast<std::string> (argument[0]));

  for (size_t n = 0; n < 10; ++n) {
    V.save (argument[0]);
    V.clear ();
    V.load (argument[0]);
    if (V != V_orig)
      FAIL ("difference detected in run " + str(n) + " for Math::Vector!");
  }




  Math::Matrix<value_type> M_orig (100, 100);
  for (size_t i = 0; i < M_orig.rows(); ++i)
    for (size_t j = 0; j < M_orig.columns(); ++j)
      M_orig(i,j) = rng.normal();
  M_orig.save (argument[0]);

  Math::Matrix<value_type> M (static_cast<std::string> (argument[0]));

  for (size_t n = 0; n < 10; ++n) {
    M.save (argument[0]);
    M.clear ();
    M.load (argument[0]);
    if (M != M_orig)
      FAIL ("difference detected in run " + str(n) + " for Math::Matrix!");
  }
}

