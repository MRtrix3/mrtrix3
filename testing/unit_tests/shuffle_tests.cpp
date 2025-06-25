/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "exception.h"
#include "math/factorial.h"
#include "math/stats/shuffle.h"
#include "math/stats/typedefs.h"
#include "types.h"

#include <Eigen/Core>
#include <algorithm>
#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <set>
#include <vector>

using namespace MR;
using namespace App;
using namespace Math::Stats;
using MR::Math::Stats::index_array_type;
using MR::Math::Stats::matrix_type;

constexpr size_t ROWS = 6;
const index_array_type BLOCK_INDICES = (index_array_type(ROWS) << 0, 1, 0, 1, 2, 2).finished();

enum class Exchange : uint8_t { NONE, WITHIN, WHOLE };
const std::vector<std::string> exchange_strings{"Unrestricted", "WithinBlock", "WholeBlock"};

namespace {
// Struct to hold parameters for each test instance
struct ShufflerParams {
  std::string name;
  size_t requested_shuffles;
  Shuffler::error_t error_type;
  Exchange exchange_type;
  bool test_uniqueness;
};

std::vector<ShufflerParams> GetShufflerTestParams() {
  std::vector<ShufflerParams> all_params;

  for (int ex_idx = 0; ex_idx < 3; ++ex_idx) {
    auto exchange_type = static_cast<Exchange>(ex_idx);
    const auto &eb_string = exchange_strings[ex_idx];

    size_t max_num_permutations = 0;
    size_t max_num_signflips = 0;
    if (exchange_type == Exchange::NONE) {
      max_num_permutations = Math::factorial(ROWS);
      max_num_signflips = size_t(1) << ROWS;
    } else if (exchange_type == Exchange::WITHIN) {
      max_num_permutations =
          static_cast<size_t>(Math::factorial(2)) * Math::factorial(2) * Math::factorial(2); // 2 per block
      max_num_signflips = size_t(1) << ROWS;
    } else {
      // WHOLE
      max_num_permutations = Math::factorial(3); // 3 blocks
      max_num_signflips = size_t(1) << 3;
    }
    const size_t max_num_combined = max_num_permutations * max_num_signflips;

    // EE and ISE tests
    for (int err_idx = 0; err_idx < 2; ++err_idx) {
      auto error_type = (err_idx != 0) ? Shuffler::error_t::ISE : Shuffler::error_t::EE;
      const auto *error_string = (err_idx != 0) ? "ISE" : "EE";
      const size_t max_num = (err_idx != 0) ? max_num_signflips : max_num_permutations;

      all_params.push_back(
          {eb_string + "_" + error_string + "_LessThanMax", max_num / 2, error_type, exchange_type, true});
      all_params.push_back({eb_string + "_" + error_string + "_ExactlyMax", max_num, error_type, exchange_type, true});
      all_params.push_back(
          {eb_string + "_" + error_string + "_MoreThanMax", 2 * max_num, error_type, exchange_type, true});
    }

    // BOTH tests
    auto error_type = Shuffler::error_t::BOTH;
    all_params.push_back(
        {eb_string + "_BOTH_LessThanMaxSignflips", max_num_signflips / 2, error_type, exchange_type, true});
    all_params.push_back({eb_string + "_BOTH_ExactlyMaxSignflips", max_num_signflips, error_type, exchange_type, true});
    all_params.push_back({eb_string + "_BOTH_BetweenSignflipsAndPermutations",
                          (max_num_signflips + max_num_permutations) / 2,
                          error_type,
                          exchange_type,
                          true});
    all_params.push_back(
        {eb_string + "_BOTH_ExactlyMaxPermutations", max_num_permutations, error_type, exchange_type, true});
    all_params.push_back({eb_string + "_BOTH_BetweenPermutationsAndCombined",
                          (max_num_permutations + max_num_combined) / 2,
                          error_type,
                          exchange_type,
                          false});
    all_params.push_back({eb_string + "_BOTH_ExactlyMaxCombined", max_num_combined, error_type, exchange_type, true});
    all_params.push_back(
        {eb_string + "_BOTH_MoreThanMaxCombined", 2 * max_num_combined, error_type, exchange_type, true});
  }

  return all_params;
}
} // namespace

class ShufflerTest : public ::testing::Test {
protected:
  void SetUp() override {
    dummy_data.resize(ROWS);
    for (ssize_t row = 0; row != ROWS; ++row)
      dummy_data[row] = default_type(row + 1);

    block_indices = BLOCK_INDICES;
    ASSERT_EQ(block_indices.size(), ROWS);

    blocks.resize(block_indices.maxCoeff() + 1);
    for (ssize_t i = 0; i != block_indices.size(); ++i)
      blocks[block_indices[i]].insert(i);
  }

  void TestPermutationWithin(Shuffler &in, const std::string &fail_msg) {
    in.reset();
    Shuffle shuffle;
    Eigen::Array<int, Eigen::Dynamic, 1> shuffled_data;
    while (in(shuffle)) {
      shuffled_data = (shuffle.data * dummy_data.matrix()).cast<int>();
      for (size_t i = 0; i != ROWS; ++i) {
        if (block_indices[std::abs(shuffled_data[static_cast<Eigen::Index>(i)]) - 1] != block_indices[i]) {
          GTEST_FAIL() << fail_msg << ": permutation occurred outside of defined block.";
        }
      }
    }
  }

  void TestSignflipWhole(Shuffler &in, const std::string &fail_msg) {
    in.reset();
    Shuffle shuffle;
    Eigen::Array<int, Eigen::Dynamic, 1> shuffled_data;
    while (in(shuffle)) {
      shuffled_data = (shuffle.data * dummy_data.matrix()).cast<int>();
      for (const auto &b : blocks) {
        auto it = b.begin();
        const bool flipped = shuffled_data[*it] < 0.0;
        for (++it; it != b.end(); ++it) {
          if (bool(shuffled_data[static_cast<Eigen::Index>(*it)] < 0.0) != flipped) {
            GTEST_FAIL() << fail_msg << ": sign-flip was not applied to the whole block.";
          }
        }
      }
    }
  }

  void TestPermutationWhole(Shuffler &in, const std::string &fail_msg) {
    in.reset();
    Shuffle shuffle;
    Eigen::Array<int, Eigen::Dynamic, 1> shuffled_data;
    while (in(shuffle)) {
      shuffled_data = (shuffle.data * dummy_data.matrix()).cast<int>();
      for (const auto &b1 : blocks) {
        const size_t first_in = *b1.begin();
        const size_t first_out = std::abs(shuffled_data[static_cast<Eigen::Index>(first_in)]) - 1;
        for (const auto &b2 : blocks) {
          if (b2.count(first_out) != 0U) {
            for (auto i : b1) {
              if (b2.count(std::abs(shuffled_data[static_cast<Eigen::Index>(i)]) - 1) == 0U) {
                GTEST_FAIL() << fail_msg << ": permutation did not exchange whole blocks.";
              }
            }
            break;
          }
        }
      }
    }
  }

  void TestUnique(Shuffler &in, const std::string &fail_msg) {
    in.reset();
    std::vector<Shuffle> matrices;
    Shuffle temp;
    while (in(temp)) {
      matrices.push_back(temp);
    }

    if (matrices.size() <= 1)
      return;

    for (size_t i = 0; i < matrices.size(); ++i) {
      for (size_t j = i + 1; j < matrices.size(); ++j) {
        ASSERT_NE(matrices[i].index, matrices[j].index) << fail_msg << " (duplicate shuffle index)";
        ASSERT_FALSE(matrices[i].data == matrices[j].data) << fail_msg << " (duplicate shuffle matrix data)";
      }
    }
  }

  vector_type dummy_data;
  index_array_type block_indices;
  std::vector<std::set<size_t>> blocks;
};

// Parameterized test fixture
class ShufflerParamTest : public ShufflerTest, public ::testing::WithParamInterface<ShufflerParams> {};

TEST_P(ShufflerParamTest, VerifyShufflingMechanisms) {
  const auto &params = GetParam();

  const index_array_type eb_within(params.exchange_type == Exchange::WITHIN ? block_indices : index_array_type());
  const index_array_type eb_whole(params.exchange_type == Exchange::WHOLE ? block_indices : index_array_type());

  size_t max_num_permutations = 0;
  size_t max_num_signflips = 0;
  switch (params.exchange_type) {
  case Exchange::WITHIN:
    max_num_permutations = 1;
    for (const auto &b : blocks)
      max_num_permutations *= Math::factorial(b.size());
    max_num_signflips = size_t(1) << ROWS;
    break;
  case Exchange::WHOLE:
    max_num_permutations = Math::factorial(blocks.size());
    max_num_signflips = size_t(1) << blocks.size();
    break;
  case Exchange::NONE:
  default:
    max_num_permutations = Math::factorial(ROWS);
    max_num_signflips = size_t(1) << ROWS;
    break;
  }
  const size_t max_num_combined = max_num_permutations * max_num_signflips;

  size_t max_possible_shuffles = 0;
  switch (params.error_type) {
  case Shuffler::error_t::EE:
    max_possible_shuffles = max_num_permutations;
    break;
  case Shuffler::error_t::ISE:
    max_possible_shuffles = max_num_signflips;
    break;
  case Shuffler::error_t::BOTH:
    max_possible_shuffles = max_num_combined;
    break;
  }

  const size_t expected_number = std::min(params.requested_shuffles, max_possible_shuffles);
  const std::string fail_msg = "Test failed for: " + params.name;

  const LogLevelLatch latch(params.requested_shuffles > expected_number ? 0 : App::log_level);
  Shuffler shuffler(ROWS, params.requested_shuffles, params.error_type, false, eb_within, eb_whole);

  ASSERT_EQ(shuffler.size(), expected_number) << fail_msg << " (incorrect number of shuffles)";

  if (eb_within.size() != 0) {
    TestPermutationWithin(shuffler, fail_msg + " (broken within-block permutation)");
  }
  if (eb_whole.size() != 0) {
    if (params.error_type == Shuffler::error_t::EE || params.error_type == Shuffler::error_t::BOTH)
      TestPermutationWhole(shuffler, fail_msg + " (broken whole-block exchangeability)");
    if (params.error_type == Shuffler::error_t::ISE || params.error_type == Shuffler::error_t::BOTH)
      TestSignflipWhole(shuffler, fail_msg + " (broken whole-block sign-flipping)");
  }

  if (params.test_uniqueness) {
    TestUnique(shuffler, fail_msg);
  }
}

// Instantiate the test suite with all generated parameter sets
INSTANTIATE_TEST_SUITE_P(ShufflerTests,
                         ShufflerParamTest,
                         ::testing::ValuesIn(GetShufflerTestParams()),
                         [](const ::testing::TestParamInfo<ShufflerParams> &info) { return info.param.name; });
