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
#include "exception.h"
#include "math/rng.h"
#include "misc/bitset.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";
  SYNOPSIS = "Verify correct operation of the BitSet class";
  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}


void run ()
{
  vector<std::string> failed_tests;
  auto test = [&] (const bool result, const std::string msg) {
    if (!result)
      failed_tests.push_back (msg);
  };
  auto identical = [] (const BitSet& a, const BitSet& b, const size_t bits) {
    for (size_t i = 0; i != bits; ++i) {
      if (a[i] != b[i])
        return false;
    }
    return true;
  };
  auto valid_last = [] (const BitSet& a, const size_t from_bit, const bool value) {
    for (size_t i = from_bit; i != a.size(); ++i) {
      if (a[i] != value)
        return false;
    }
    return true;
  };
  // Test up to two complete bytes; anything beyond that shouldn't be testing anything new
  for (size_t bits = 0; bits != 16; ++bits) {
    Math::RNG::Integer<size_t> rng (bits ? (bits-1) : 0);
    // Complete the sequence of tests twice:
    //   Once filling the bits with 0 and then setting specific bits to 1,
    //   then again filling them with 1 and then setting specific bits to 0
    for (size_t iter_fillvalue = 0; iter_fillvalue != 2; ++iter_fillvalue) {
      const bool fill_value = bool(iter_fillvalue);
      const bool set_value = !fill_value;
      BitSet data (bits, fill_value);
      switch (iter_fillvalue) {
        case 0: test (data.empty(), "Zero-filled set of size " + str(bits) + " reported non-empty"); break;
        case 1: test (data.full(), "One-filled set of size " + str(bits) + " reported non-full"); break;
        default: assert (0);
      }
      for (size_t i = 0; i != bits; ++i) {
        size_t index_to_toggle;
        do {
          index_to_toggle = rng();
        } while (data[index_to_toggle] != fill_value);
        switch (iter_fillvalue) {
          case 0: test (!data.full(), "Incompletely filled set of size " + str(bits) + " reported full"); break;
          case 1: test (!data.empty(), "Incompletely zeroed set of size " + str(bits) + " reported empty"); break;
          default: assert (0);
        }
        data[index_to_toggle] = set_value;
        const size_t target_count = iter_fillvalue ? (bits-(i+1)) : (i+1);
        test (data.count() == target_count, "Data " + str(data) + " (count " + str(target_count) + ") erroneously reported as count " + str(data.count()));
      }
      switch (iter_fillvalue) {
        case 0: test (data.full(), "Progressively filled set of size " + str(bits) + " reported non-full"); break;
        case 1: test (data.empty(), "Progressively zeroed set of size " + str(bits) + " reported non-empty"); break;
        default: assert (0);
      }
      // Duplicate
      BitSet duplicate (data);
      test (identical (data, duplicate, bits), "Duplicated sets pre-resize of size " + str(bits) + " did not lead to identical data: " + str(data) + " " + str(duplicate));
      // Change one bit within the final byte; make sure duplicate is no longer equivalent according to operator==()
      if (bits) {
        size_t index_to_toggle;
        do {
          index_to_toggle = rng();
        } while (index_to_toggle < 8*((bits-1)/8));
        duplicate[index_to_toggle] = fill_value;
        test (data != duplicate, "Change of one bit in size " + str(bits) + " not reported as inequal: " + str(data) + " " + str(duplicate));
        duplicate[index_to_toggle] = set_value;
        test (data == duplicate, "Reversion of one changed bit in size " + str(bits) + " not reported as equal: " + str(data) + " " + str(duplicate));
      }
      // Resize, make sure data are preserved and new items are set appropriately
      data.resize (bits+8, false);
      duplicate.resize (bits+8, true);
      test (identical (data, duplicate, bits), "Duplicated sets post-resize of size " + str(bits) + " did not lead to identical data: " + str(data) + " " + str(duplicate));
      switch (iter_fillvalue) {
        case 0:
          test (valid_last (data, bits, false), "Resized (0's) full set of size " + str(bits) + " contains invalid zeroed excess data: " + str(data));
          test (valid_last (duplicate, bits, true), "Resized (1's) full set of size " + str(bits) + " contains invalid filled excess data: " + str(duplicate));
          break;
        case 1:
          test (valid_last (data, bits, false), "Resized (0's) empty set of size " + str(bits) + " contains invalid zeroed excess data: " + str(data));
          test (valid_last (duplicate, bits, true), "Resized (1's) empty set of size " + str(bits) + " contains invalid filled excess data: " + str(duplicate));
          break;
        default: assert (0);
      }
    }
  }
  if (failed_tests.size()) {
    Exception e (str(failed_tests.size()) + " tests of BitSet class failed:");
    for (auto s : failed_tests)
      e.push_back (s);
    throw e;
  }
}

