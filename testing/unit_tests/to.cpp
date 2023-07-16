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
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Test the to<>(std::string) function";

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}


vector<std::string> failures;


template <class T>
void test (const vector<std::string>& strings, const vector<bool>& results)
{
  for (size_t i = 0; i != strings.size(); ++i) {
    try {
      to<T> (strings[i]);
      if (!results[i])
        failures.push_back (strings[i] + " to " + typeid(T).name() + " succeeded");
    } catch (Exception& e) {
      if (results[i])
        failures.push_back (strings[i] + " to " + typeid(T).name() + " failed: " + e[0]);
    }
  }
}



void run ()
{
  const vector<std::string> data = {
    "0",
    "1",
    "2",
    "0 ",
    " 1",
    "0 0",
    "0a",
    "a0",
    "true",
    "TRUE",
    "tru",
    "truee",
    "false",
    "FALSE",
    "fals",
    "falsee",
    "true ",
    "yes",
    "YES",
    "yeah",
    "yess",
    "no",
    "NO",
    "nope",
    "na",
    "0.0",
    "1e",
    "1e-1",
    "1e-1a",
    "inf",
    "INF",
    "infinity",
    "-inf",
    "-infinity",
    "nan",
    "NAN",
    "nana",
    "-nan",
    "i",
    "I",
    "j",
    "J",
    "-i",
    "1i",
    "1i0",
    "1+i",
    "1+ii",
    "a1+i",
    "1+1+i",
    "-1-i",
    "inf+infi",
    " -inf+-nani " };

  const vector<bool> bool_tests = {
    true,  // "0"
    true,  // "1"
    true,  // "2"
    true,  // "0 "
    true,  // " 1"
    false, // "0 0"
    false, // "0a"
    false, // "a0"
    true,  // "true"
    true,  // "TRUE"
    false, // "tru"
    false, // "truee"
    true,  // "false"
    true,  // "FALSE"
    false, // "fals"
    false, // "falsee"
    true,  // "true "
    true,  // "yes"
    true,  // "YES"
    false, // "yeah"
    false, // "yess"
    true,  // "no"
    true,  // "NO"
    false, // "nope"
    false, // "na"
    false, // "0.0"
    false, // "1e"
    false, // "1e-1"
    false, // "1e-1a"
    false, // "inf"
    false, // "INF"
    false, // "infinity"
    false, // "-inf"
    false, // "-infinity"
    false, // "nan"
    false, // "NAN"
    false, // "nana"
    false, // "-nan"
    false, // "i"
    false, // "I"
    false, // "j"
    false, // "J"
    false, // "-i"
    false, // "1i"
    false, // "1i0"
    false, // "1+i"
    false, // "1+ii"
    false, // "a1+i"
    false, // "1+1+i"
    false, // "-1-i"
    false, // "inf+infi"
    false  // " -inf+-nani "
  };

  const vector<bool> int_tests = {
      true,  // "0"
      true,  // "1"
      true,  // "2"
      true,  // "0 "
      true,  // " 1"
      false, // "0 0"
      false, // "0a"
      false, // "a0"
      false, // "true"
      false, // "TRUE"
      false, // "tru"
      false, // "truee"
      false, // "false"
      false, // "FALSE"
      false, // "fals"
      false, // "falsee"
      false, // "true "
      false, // "yes"
      false, // "YES"
      false, // "yeah"
      false, // "yess"
      false, // "no"
      false, // "NO"
      false, // "nope"
      false, // "na"
      false, // "0.0"
      false, // "1e"
      false, // "1e-1"
      false, // "1e-1a"
      false, // "inf"
      false, // "INF"
      false, // "infinity"
      false, // "-inf"
      false, // "-infinity"
      false, // "nan"
      false, // "NAN"
      false, // "nana"
      false, // "-nan"
      false, // "i"
      false, // "I"
      false, // "j"
      false, // "J"
      false, // "-i"
      false, // "1i"
      false, // "1i0"
      false, // "1+i"
      false, // "1+ii"
      false, // "a1+i"
      false, // "1+1+i"
      false, // "-1-i"
      false, // "inf+infi"
      false  // " -inf+-nani "
  };

  const vector<bool> float_tests = {
      true,  // "0"
      true,  // "1"
      true,  // "2"
      true,  // "0 "
      true,  // " 1"
      false, // "0 0"
      false, // "0a"
      false, // "a0"
      false, // "true"
      false, // "TRUE"
      false, // "tru"
      false, // "truee"
      false, // "false"
      false, // "FALSE"
      false, // "fals"
      false, // "falsee"
      false, // "true "
      false, // "yes"
      false, // "YES"
      false, // "yeah"
      false, // "yess"
      false, // "no"
      false, // "NO"
      false, // "nope"
      false, // "na"
      true,  // "0.0"
      false, // "1e"
      true,  // "1e-1"
      false, // "1e-1a"
      true,  // "inf"
      true,  // "INF"
      false, // "infinity"
      true,  // "-inf"
      false, // "-infinity"
      true,  // "nan"
      true,  // "NAN"
      false, // "nana"
      true,  // "-nan"
      false, // "i"
      false, // "I"
      false, // "j"
      false, // "J"
      false, // "-i"
      false, // "1i"
      false, // "1i0"
      false, // "1+i"
      false, // "1+ii"
      false, // "a1+i"
      false, // "1+1+i"
      false, // "-1-i"
      false, // "inf+infi"
      false  // " -inf+-nani "
  };

  const vector<bool> complex_tests = {
      true,  // "0"
      true,  // "1"
      true,  // "2"
      true,  // "0 "
      true,  // " 1"
      false, // "0 0"
      false, // "0a"
      false, // "a0"
      false, // "true"
      false, // "TRUE"
      false, // "tru"
      false, // "truee"
      false, // "false"
      false, // "FALSE"
      false, // "fals"
      false, // "falsee"
      false, // "true "
      false, // "yes"
      false, // "YES"
      false, // "yeah"
      false, // "yess"
      false, // "no"
      false, // "NO"
      false, // "nope"
      false, // "na"
      true,  // "0.0"
      false, // "1e"
      true,  // "1e-1"
      false, // "1e-1a"
      true,  // "inf"
      true,  // "INF"
      false, // "infinity"
      true,  // "-inf"
      false, // "-infinity"
      true,  // "nan"
      true,  // "NAN"
      false, // "nana"
      true,  // "-nan"
      true,  // "i"
      false, // "I"
      true,  // "j"
      false, // "J"
      true,  // "-i"
      true,  // "1i"
      false, // "1i0"
      true,  // "1+i"
      false, // "1+ii"
      false, // "a1+i"
      false, // "1+1+i"
      true,  // "-1-i"
      true,  // "inf+infi"
      true   //" -inf+-nani "
  };

  test<bool> (data, bool_tests);
  test<int>  (data, int_tests);
  test<float> (data, float_tests);
  test<std::complex<float>> (data, complex_tests);

  if (failures.size()) {
    Exception e (str(failures.size()) + " of " + str(4*data.size()) + " tests failed:");
    for (const auto& s : failures)
      e.push_back (s);
    throw e;
  }

  CONSOLE ("All tests passed OK");

}

