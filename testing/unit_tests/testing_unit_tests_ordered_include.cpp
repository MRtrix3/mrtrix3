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


#include "command.h"
#include "unit_tests/tractography/roi_unit_tests.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Lee Reid (lee.reid@csiro.au)";

  SYNOPSIS = "Runs units tests for tractography-related classes";

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
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
