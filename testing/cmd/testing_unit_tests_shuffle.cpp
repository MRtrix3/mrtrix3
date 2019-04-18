/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "command.h"
#include "exception.h"
#include "types.h"
#include "math/factorial.h"
#include "math/math.h"
#include "math/stats/shuffle.h"
#include "math/stats/typedefs.h"

using namespace MR;
using namespace App;
using namespace Math::Stats;
using MR::Math::Stats::matrix_type;

#define ROWS size_t(4)

void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";
  SYNOPSIS = "Verify correct operation of shuffling mechanisms for permutation testing";
  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}




void run ()
{
  vector<std::string> failed_tests;

  auto test = [&] (const bool result, const std::string msg) {
    if (!result)
      failed_tests.push_back (msg);
  };

  auto test_unique = [&] (Shuffler& in, const std::string& msg) {
    vector<Shuffle> matrices;
    Shuffle temp;
    bool duplicate_index = false, duplicate_data = false;
    while (in (temp)) {
      for (const auto& previous : matrices) {
        if (temp.index == previous.index)
          duplicate_index = true;
        if (temp.data == previous.data)
          duplicate_data = true;
        matrices.push_back (temp);
      }
    }
    if (duplicate_index)
      failed_tests.push_back (msg + " (duplicate shuffle index)");
    if (duplicate_data)
      failed_tests.push_back (msg + " (duplicate shuffle matrix data)");
  };

  auto test_kernel = [&] (const size_t requested_number,
                          const size_t expected_number,
                          const Shuffler::error_t error_type,
                          const std::string& error_string,
                          const std::string& test_string,
                          const bool test_uniqueness)
  {
    LogLevelLatch latch (requested_number > expected_number ? 0 : App::log_level);
    Shuffler temp (ROWS, requested_number, error_type, false);
    test (temp.size() == expected_number, "Incorrect number of shuffles; " + error_string + "; " + test_string);
    if (test_uniqueness)
      test_unique (temp, "Bad shuffles; " + error_string + "; " + test_string);
  };

  const size_t max_num_permutations = Math::factorial (ROWS);
  const size_t max_num_signflips = size_t(1) << ROWS;
  const size_t max_num_combined = max_num_permutations * max_num_signflips;

  // EE and ISE
  for (size_t e = 0; e != 2; ++e) {
    const Shuffler::error_t error_type (e ? Shuffler::error_t::ISE : Shuffler::error_t::EE);
    const std::string error_string (e ? "ISE" : "EE");
    const size_t max_num (e ? max_num_signflips : max_num_permutations);
    test_kernel (max_num/2, max_num/2, error_type, error_string, "less than max shuffles", true);
    test_kernel (max_num, max_num, error_type, error_string, "exactly max shuffles", true);
    test_kernel (2*max_num, max_num, error_type, error_string, "more than max shuffles", true);
  }

  // BOTH
  {
    test_kernel (max_num_signflips/2, max_num_signflips/2, Shuffler::error_t::BOTH, "BOTH", "less than max signflips", true);
    test_kernel (max_num_signflips, max_num_signflips, Shuffler::error_t::BOTH, "BOTH", "exactly max signflips", true);
    test_kernel ((max_num_signflips + max_num_permutations)/2, (max_num_signflips + max_num_permutations)/2, Shuffler::error_t::BOTH, "BOTH", "between max signflips and max permutations", true);
    test_kernel (max_num_permutations, max_num_permutations, Shuffler::error_t::BOTH, "BOTH", "exactly max permutations", true);
    // Note: Only test where uniqueness of shuffles is not guaranteed
    //   (both signflips and permutations will individually have random duplicates)
    test_kernel ((max_num_permutations + max_num_combined)/2, (max_num_permutations + max_num_combined)/2, Shuffler::error_t::BOTH, "BOTH", "between max permutations and max shuffles", false);
    test_kernel (max_num_combined, max_num_combined, Shuffler::error_t::BOTH, "BOTH", "exactly max shuffles", true);
    test_kernel (2 * max_num_combined, max_num_combined, Shuffler::error_t::BOTH, "BOTH", "more than max shuffles", true);
  }

  if (failed_tests.size()) {
    Exception e (str(failed_tests.size()) + " tests of shuffling mechanisms failed:");
    for (auto s : failed_tests)
      e.push_back (s);
    throw e;
  }
}

