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
#include "debug.h"
#include "dwi/sdeconv/response.h"


using namespace MR;
using namespace App;




void usage ()
{
  DESCRIPTION
    + "test response function class";

  ARGUMENTS
    + Argument ("b-vals", "the b-values at which to evaluate the response function.").type_file();

  OPTIONS
    + Option ("response", "the file from which to read the response function coefficients, if not using default values")
    +   Argument ("file").type_file();
}






void run ()
{
  DWI::Response<float> response;
  Options opt = get_options ("response");
  if (opt.size())
    response.load (opt[0][0]);

  Math::Vector<float> bvals;
  bvals.load (argument[0]);

  for (size_t n = 0; n < bvals.size(); ++n) {
    response.set_bval (bvals[n]);
    for (size_t l = 0; l <= response.lmax(); l+=2)
      std::cout << response.value (l) << " ";
    std::cout << "\n";
  }
}

