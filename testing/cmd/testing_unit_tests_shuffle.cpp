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


#include <set>

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
using MR::Math::Stats::index_array_type;

#define ROWS size_t(6)
#define BLOCK_INDICES 0,1,0,1,2,2 // Must increment from zero, and must be equal number in each
enum exchange_t { NONE, WITHIN, WHOLE };
vector<std::string> exchange_strings { "Unrestricted", "within-block", "whole-block" };

void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";
  SYNOPSIS = "Verify correct operation of shuffling mechanisms for permutation testing";
  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}



void run ()
{
  vector<std::string> failed_tests;

  vector_type dummy_data (ROWS);
  for (ssize_t row = 0; row != ROWS; ++row)
    dummy_data[row] = default_type(row+1);

  index_array_type block_indices (ROWS);
  block_indices << BLOCK_INDICES;
  assert (block_indices.size() == ROWS);
  vector<std::set<size_t>> blocks (block_indices.maxCoeff()+1);
  for (ssize_t i = 0; i != block_indices.size(); ++i)
    blocks[block_indices[i]].insert (i);

  auto test = [&] (const bool result, const std::string msg)
  {
    if (!result)
      failed_tests.push_back (msg);
  };

  auto test_permutation_within = [&] (Shuffler& in, const std::string& msg)
  {
    in.reset();
    Shuffle shuffle;
    Eigen::Array<int, Eigen::Dynamic, 1> shuffled_data;
    while (in (shuffle)) {
      shuffled_data = (shuffle.data * dummy_data.matrix()).cast<int>();
      for (size_t i = 0; i != ROWS; ++i) {
        if (block_indices[std::abs(shuffled_data[i])-1] != block_indices[i]) {
          failed_tests.push_back (msg);
          return;
        }
      }
    }
  };

  auto test_signflip_whole = [&] (Shuffler& in, const std::string& msg)
  {
    in.reset();
    Shuffle shuffle;
    Eigen::Array<int, Eigen::Dynamic, 1> shuffled_data;
    while (in (shuffle)) {
      shuffled_data = (shuffle.data * dummy_data.matrix()).cast<int>();
      for (const auto& b : blocks) {
        // Ensure that either all values in the block have been flipped,
        //   or none have been flipped
        auto it = b.begin();
        const bool flipped = shuffled_data[*it] < 0.0;
        for (++it; it != b.end(); ++it) {
          if (bool (shuffled_data[*it] < 0.0) != flipped) {
            failed_tests.push_back (msg);
            return;
          }
        }
      }
    }
  };

  auto test_permutation_whole = [&] (Shuffler& in, const std::string& msg)
  {
    in.reset();
    Shuffle shuffle;
    Eigen::Array<int, Eigen::Dynamic, 1> shuffled_data;
    while (in (shuffle)) {
      shuffled_data = (shuffle.data * dummy_data.matrix()).cast<int>();
      for (const auto& b1 : blocks) {
        // Only test each block once; use the first index within the block
        const size_t first_in = *b1.begin();
        // What got mapped into this location by the shuffling?
        const size_t first_out = std::abs (shuffled_data[first_in]) - 1;
        // Find the block from which this value originated
        for (const auto& b2 : blocks) {
          if (b2.find (first_out) != b2.end()) {
            // Index at the start of block b1 in the shuffled data originated from block b2 before shuffling
            // Ensure that ALL indices in block b1 in the shuffled data originated from block b2
            for (auto i : b1) {
              if (b2.find (std::abs (shuffled_data[i])-1) == b2.end()) {
                failed_tests.push_back (msg);
                return;
              }
            }
            break;
          }
        }

      }
    }
  };

  auto test_unique = [&] (Shuffler& in, const std::string& msg)
  {
    in.reset();
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
                          const index_array_type& eb_within,
                          const index_array_type& eb_whole,
                          const std::string& error_string,
                          const std::string& eb_string,
                          const std::string& test_string,
                          const bool test_uniqueness)
  {
    LogLevelLatch latch (requested_number > expected_number ? 0 : App::log_level);
    Shuffler temp (ROWS, requested_number, error_type, false, eb_within, eb_whole);
    test (temp.size() == expected_number, "Incorrect number of shuffles; " + error_string + "; " + eb_string + "; " + test_string);
    if (eb_within.size())
      test_permutation_within (temp, "Broken within-block permutation; " + error_string + "; " + test_string);
    if (eb_whole.size()) {
      if (error_type == Shuffler::error_t::EE || error_type == Shuffler::error_t::BOTH)
        test_permutation_whole (temp, "Broken whole-block exchangeability; " + error_string + "; " + test_string);
      if (error_type == Shuffler::error_t::ISE || error_type == Shuffler::error_t::BOTH)
        test_signflip_whole (temp, "Broken whole-block sign-flipping; " + error_string + "; " + test_string);
    }
    if (test_uniqueness)
      test_unique (temp, "Bad shuffles; " + error_string + "; " + eb_string + "; " + test_string);
  };

  for (size_t exchange_index = 0; exchange_index != 3; ++exchange_index) {

    const index_array_type eb_within (exchange_t(exchange_index) == exchange_t::WITHIN ?
                                      block_indices :
                                      index_array_type());
    const index_array_type eb_whole  (exchange_t(exchange_index) == exchange_t::WHOLE ?
                                      block_indices :
                                      index_array_type());
    const std::string eb_string (exchange_strings[exchange_index]);

    size_t max_num_permutations, max_num_signflips;
    switch (exchange_index) {
      case exchange_t::NONE:
        max_num_permutations = Math::factorial (ROWS);
        max_num_signflips = size_t(1) << ROWS;
        break;
      case exchange_t::WITHIN:
        max_num_permutations = 1;
        for (const auto& b : blocks)
          max_num_permutations *= Math::factorial (b.size());
        max_num_signflips = size_t(1) << ROWS;
        break;
      case exchange_t::WHOLE:
        max_num_permutations = Math::factorial (blocks.size());
        max_num_signflips = size_t(1) << blocks.size();
        break;
    }
    const size_t max_num_combined = max_num_permutations * max_num_signflips;

    // EE and ISE
    for (size_t error_index = 0; error_index != 2; ++error_index) {
      const Shuffler::error_t error_type (error_index ? Shuffler::error_t::ISE : Shuffler::error_t::EE);
      const std::string error_string (error_index ? "ISE" : "EE");
      const size_t max_num (error_index ? max_num_signflips : max_num_permutations);
      test_kernel (max_num/2, max_num/2, error_type, eb_within, eb_whole, error_string, eb_string, "less than max shuffles", true);
      test_kernel (max_num, max_num, error_type, eb_within, eb_whole, error_string, eb_string, "exactly max shuffles", true);
      test_kernel (2*max_num, max_num, error_type, eb_within, eb_whole, error_string, eb_string, "more than max shuffles", true);
    }

    // BOTH
    {
      test_kernel (max_num_signflips/2, max_num_signflips/2, Shuffler::error_t::BOTH, eb_within, eb_whole, "BOTH", eb_string, "less than max signflips", true);
      test_kernel (max_num_signflips, max_num_signflips, Shuffler::error_t::BOTH, eb_within, eb_whole, "BOTH", eb_string, "exactly max signflips", true);
      test_kernel ((max_num_signflips + max_num_permutations)/2, (max_num_signflips + max_num_permutations)/2, Shuffler::error_t::BOTH, eb_within, eb_whole, "BOTH", eb_string, "between max signflips and max permutations", true);
      test_kernel (max_num_permutations, max_num_permutations, Shuffler::error_t::BOTH, eb_within, eb_whole, "BOTH", eb_string, "exactly max permutations", true);
      // Note: Only test where uniqueness of shuffles is not guaranteed
      //   (both signflips and permutations will individually have random duplicates)
      test_kernel ((max_num_permutations + max_num_combined)/2, (max_num_permutations + max_num_combined)/2, Shuffler::error_t::BOTH, eb_within, eb_whole, "BOTH", eb_string, "between max permutations and max shuffles", false);
      test_kernel (max_num_combined, max_num_combined, Shuffler::error_t::BOTH, eb_within, eb_whole, "BOTH", eb_string, "exactly max shuffles", true);
      test_kernel (2 * max_num_combined, max_num_combined, Shuffler::error_t::BOTH, eb_within, eb_whole, "BOTH", eb_string, "more than max shuffles", true);
    }

  }

  if (failed_tests.size()) {
    Exception e (str(failed_tests.size()) + " tests of shuffling mechanisms failed:");
    for (auto s : failed_tests)
      e.push_back (s);
    throw e;
  }
}

