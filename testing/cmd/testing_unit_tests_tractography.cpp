/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "command.h"
#include "progressbar.h"
#include "datatype.h"
#include "math/rng.h"

#include "image.h"
#include "unit_tests/tractography/roi_unit_tests.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Lee Reid (lee.reid@csiro.au)";

  SYNOPSIS = "Runs units tests for tractography-related classes";

  ARGUMENTS
     + Argument("run", "run the tests (else just show this message)");
}


void run ()
{

     bool allPassed = Testing::UnitTests::Tractography::ROIUnitTests::ROIUnitTests::run();
     //Add further tests here like so: 
     //allPassed &= <test>
     //OR
     //allPassed = allPassed && <test> 


     if (allPassed)
     {
        std::cout << "All Passed";
     }
     else
     {
        std::cout << "Failed";
        throw 1;//Register an error - command.h doesn't let us return a variable - it only listens for something being thrown 
     }   
}
