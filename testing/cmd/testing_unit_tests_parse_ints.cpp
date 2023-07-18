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

#include <complex>

#include "command.h"
#include "mrtrix.h"
#include "types.h"


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Test the parse_ints (std::string) function";

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}


vector<std::string> failures;




void run ()
{

  struct Test {
    const char* str;
    std::vector<int> result;
  } tests[] = {

    { "1", { 1 } },
    { "1,3,4", { 1,3,4 } },
    { "5:9", { 5,6,7,8,9 } },
    { "2:2:10", { 2,4,6,8,10 } },
    { "6:3:-6", { 6,3,0,-3,-6 } },
    { "1:3,5:7", { 1,2,3,5,6,7 } },
    { "1:2:10,20:5:-7", { 1,3,5,7,9,20,15,10,5,0,-5 } },
    { "abc", { } },
    { "a,b,c", { } },
    { "1,3,c", { } },
    { "1:3,c", { } },
    { "1, 5, 7", { 1,5,7 } },
    { "1 5 7", { 1,5,7 } },
    { "1,\t   5\t7", { 1,5,7 } },
    { "1:  5, 7", { 1,2,3,4,5,7 } },
    { "1: 5 7", { 1,2,3,4,5,7 } },
    { "1 :5 7", { 1,2,3,4,5,7 } },
    { "1 : 2 : 5 7", { 1,3,5,7 } },
    { "1 :2 :-5 7", { 1,-1,-3,-5,7 } },
    { "1 : 2: 11 20: 3 :30", { 1,3,5,7,9,11,20,23,26,29 } },

  };

  for (const auto& t : tests) {
    try {
      if (parse_ints<int> (t.str) != t.result)
        failures.push_back ("\"" + std::string (t.str) + "\" to " + str(t.result) + " failed (produced " + str(parse_ints<int>(t.str)) + ")");
    }
    catch (Exception& e) {
      if (t.result.size())
        failures.push_back ("\"" + std::string (t.str) + "\" to " + str(t.result) + " failed: " + e[0]);
    }
  }

  if (failures.size()) {
    Exception e (str(failures.size()) + " of " + str(sizeof(tests)/sizeof(Test)) + " tests failed:");
    for (const auto& s : failures)
      e.push_back (s);
    throw e;
  }

  CONSOLE ("All tests passed OK");

}


