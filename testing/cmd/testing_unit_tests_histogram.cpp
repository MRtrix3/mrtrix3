/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include "algo/histogram.h"
#include "command.h"
#include "exception.h"

using namespace MR;
using namespace App;
using namespace MR::Algo::Histogram;

void usage ()
{
  AUTHOR = "MRtrix3 contributors";

  SYNOPSIS = "Verify that Histogram::Data does not drop the maximum-valued input from its bins";

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}


vector<std::string> failures;


// (min, max, num_bins): floating-point division can make (max - min) / bin_width
//   round up to exactly num_bins for val == max, even though bin_width was itself
//   derived as (max - min) / num_bins. Each of these triples reproduces that rounding.
struct Case { default_type min, max; size_t num_bins; };
const Case cases[] = {
  { 0.0,   10.0,  3  },
  { 0.0,   1.0,   7  },
  { -5.0,  5.0,   4  },
  { 0.0,   100.0, 9  },
  { 1.0,   2.0,   10 },
};


void run ()
{
  for (const auto& c : cases) {

    Calibrator calibrator (c.num_bins);
    calibrator (c.min);
    calibrator (c.max);
    calibrator.finalize (1, false);

    const std::string label = "min=" + str(c.min) + " max=" + str(c.max) + " num_bins=" + str(c.num_bins);

    if (calibrator.get_bin_width() <= 0.0 || !std::isfinite (calibrator.get_bin_width())) {
      failures.push_back (label + ": calibrator produced an unusable bin width");
      continue;
    }

    Data data (calibrator);
    data (c.min);
    data (c.max);

    size_t total = 0;
    for (size_t i = 0; i != data.size(); ++i)
      total += data[i];

    if (total != 2)
      failures.push_back (label + ": fed 2 values but " + str(total) + " were counted across " + str(data.size()) + " bins (the maximum value was silently dropped)");

    if (data.size() && data[data.size()-1] == 0)
      failures.push_back (label + ": the maximum value did not land in the last bin");
  }

  // The overflow guard for a value genuinely beyond the calibrated range must still
  //   return the out-of-range sentinel (Data::size()), not a clamped in-range index.
  // This must hold on both sides of the range, and Data::operator() must count such
  //   values in get_num_out_of_range() rather than adding them to any bin.
  size_t extra_tests = 1;
  {
    Calibrator calibrator (4);
    calibrator (0.0);
    calibrator (10.0);
    calibrator.finalize (1, false);
    Data data (calibrator);
    const size_t pos_above = data.bin (1000.0);
    if (pos_above != data.size())
      failures.push_back ("a value far beyond the calibrated maximum returned bin index " + str(pos_above) + " instead of the out-of-range sentinel " + str(data.size()));

    ++extra_tests;
    const size_t pos_below = data.bin (-1000.0);
    if (pos_below != data.size())
      failures.push_back ("a value far below the calibrated minimum returned bin index " + str(pos_below) + " instead of the out-of-range sentinel " + str(data.size()));

    ++extra_tests;
    if (data.get_num_out_of_range() != 0)
      failures.push_back ("a freshly constructed Data already reports " + str(data.get_num_out_of_range()) + " out-of-range values");

    data (1000.0);
    data (-1000.0);
    data (5.0); // in range, must not be counted as out-of-range

    ++extra_tests;
    if (data.get_num_out_of_range() != 2)
      failures.push_back ("feeding one below-range and one above-range value left get_num_out_of_range() at " + str(data.get_num_out_of_range()) + " instead of 2");

    ++extra_tests;
    size_t total_in_range = 0;
    for (size_t i = 0; i != data.size(); ++i)
      total_in_range += data[i];
    if (total_in_range != 1)
      failures.push_back ("two out-of-range values were counted into the histogram bins (total " + str(total_in_range) + " instead of 1)");
  }

  if (failures.size()) {
    Exception e (str(failures.size()) + " of " + str(sizeof(cases)/sizeof(Case) + extra_tests) + " tests failed:");
    for (const auto& s : failures)
      e.push_back (s);
    throw e;
  }

  CONSOLE ("All tests passed OK");
}
