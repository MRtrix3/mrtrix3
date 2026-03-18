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

#include <cstddef>
#include <vector>

#include "gtest/gtest.h"

#include "exception.h"
#include "mrtrix.h"
#include "stride.h"

using namespace MR;

// Mock header class for testing
class MockHeader {
public:
  MockHeader(const std::vector<size_t> &sizes, const std::vector<ssize_t> &strides)
      : sizes_(sizes), strides_(strides) {}

  size_t ndim() const { return sizes_.size(); }
  size_t size(size_t axis) const { return sizes_[axis]; }
  ssize_t stride(size_t axis) const { return strides_[axis]; }
  // Necessary for testing legacy functions where these could be set individually
  ssize_t &stride(size_t axis) { return strides_[axis]; }

private:
  std::vector<size_t> sizes_;
  std::vector<ssize_t> strides_;
};

// =============================================================================
// Order class tests (New implementation)
// =============================================================================

class StrideOrderTest : public ::testing::Test {};

TEST_F(StrideOrderTest, ConstructFromVector) {
  Stride::Order order(std::vector<ArrayIndex>{0, 1, 2, 3});
  EXPECT_EQ(order.size(), 4);
  EXPECT_EQ(order[0], 0);
  EXPECT_EQ(order[1], 1);
  EXPECT_EQ(order[2], 2);
  EXPECT_EQ(order[3], 3);
}

TEST_F(StrideOrderTest, ConstructFromPermutation) {
  Stride::Permutation perm({0, 1, 2, 3});
  Stride::Order order(perm);
  EXPECT_EQ(order.size(), 4);
  EXPECT_TRUE(order.is_canonical());
}

TEST_F(StrideOrderTest, IsCanonical) {
  Stride::Order canonical_order(std::vector<ArrayIndex>{0, 1, 2, 3});
  EXPECT_TRUE(canonical_order.is_canonical());

  Stride::Order non_canonical(std::vector<ArrayIndex>{3, 0, 1, 2});
  EXPECT_FALSE(non_canonical.is_canonical());
}

TEST_F(StrideOrderTest, IsSanitised) {
  Stride::Order sanitised(std::vector<ArrayIndex>{0, 1, 2, 3});
  EXPECT_TRUE(sanitised.is_sanitised());

  Stride::Order invalid_min(std::vector<ArrayIndex>{1, 2, 3, 4});
  EXPECT_FALSE(invalid_min.is_sanitised());

  Stride::Order invalid_max(std::vector<ArrayIndex>{0, 1, 2, 4});
  EXPECT_FALSE(invalid_max.is_sanitised());

  Stride::Order duplicate(std::vector<ArrayIndex>{0, 1, 1, 3});
  EXPECT_FALSE(duplicate.is_sanitised());

  Stride::Order uninitialised(std::vector<ArrayIndex>{0, 1, 2, -1});
  EXPECT_FALSE(uninitialised.is_sanitised());
}

// TODO Sort out distinction between is_sanitised() and valid() for Order class
TEST_F(StrideOrderTest, Valid) {
  Stride::Order valid_order(std::vector<ArrayIndex>{2, 0, 1, 3});
  EXPECT_TRUE(valid_order.valid());

  Stride::Order duplicate(std::vector<ArrayIndex>{0, 1, 1, 3});
  EXPECT_FALSE(duplicate.valid());

  Stride::Order negative(std::vector<ArrayIndex>{-1, 0, 1, 2});
  EXPECT_FALSE(negative.valid());
}

TEST_F(StrideOrderTest, HeadAndTail) {
  Stride::Order order(std::vector<ArrayIndex>{3, 0, 1, 2});

  auto head = order.head(2);
  EXPECT_EQ(head.size(), 2);
  EXPECT_EQ(head[0], 3);
  EXPECT_EQ(head[1], 0);

  auto tail = order.tail(2);
  EXPECT_EQ(tail.size(), 2);
  EXPECT_EQ(tail[0], 1);
  EXPECT_EQ(tail[1], 2);

  EXPECT_DEATH(order.head(5), "Assertion `num_axes <= size\\(\\)' failed.");
  EXPECT_DEATH(order.tail(5), "Assertion `num_axes <= size\\(\\)' failed.");
}

TEST_F(StrideOrderTest, Subset) {
  Stride::Order order(std::vector<ArrayIndex>{3, 0, 1, 2});

  auto first_two = order.subset(0, 2);
  EXPECT_EQ(first_two.size(), 2);
  EXPECT_EQ(first_two[0], 0);
  EXPECT_EQ(first_two[1], 1);

  auto skip_first = order.subset(1);
  EXPECT_EQ(skip_first.size(), 3);
  EXPECT_EQ(skip_first[0], 3);
  EXPECT_EQ(skip_first[1], 1);
  EXPECT_EQ(skip_first[2], 2);

  EXPECT_DEATH(order.subset(-1), "Assertion `from_axis >= 0' failed.");
  EXPECT_DEATH(order.subset(2, 2), "Assertion `to_axis > from_axis' failed.");
  EXPECT_DEATH(order.subset(0, 5), "Assertion `to_axis <= size\\(\\)' failed.");
}

TEST_F(StrideOrderTest, Canonical) {
  auto order = Stride::Order::canonical(5);
  EXPECT_EQ(order.size(), 5);
  EXPECT_TRUE(order.is_canonical());
  EXPECT_TRUE(order.is_sanitised());
  for (size_t i = 0; i < 5; ++i)
    EXPECT_EQ(order[i], i);
}

// =============================================================================
// Permutation class tests (New implementation)
// =============================================================================

class StridePermutationTest : public ::testing::Test {};

TEST_F(StridePermutationTest, ConstructFromVector) {
  Stride::Permutation perm({0, 1, 2, 3});
  EXPECT_EQ(perm.size(), 4);
  EXPECT_TRUE(perm.is_canonical());
}

TEST_F(StridePermutationTest, ConstructFromOrder) {
  Stride::Order order(std::vector<ArrayIndex>{3, 0, 1, 2});
  Stride::Permutation perm(order);
  EXPECT_EQ(perm.size(), 4);
  EXPECT_EQ(perm[0], 1);
  EXPECT_EQ(perm[1], 2);
  EXPECT_EQ(perm[2], 3);
  EXPECT_EQ(perm[3], 0);
}

TEST_F(StridePermutationTest, ConstructFromSymbolicCanonical) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  Stride::Permutation perm(symbolic);
  EXPECT_EQ(perm.size(), 4);
  EXPECT_TRUE(perm.is_canonical());
}

TEST_F(StridePermutationTest, ConstructFromSymbolicInvalid) {
  Stride::Symbolic symbolic({0, 0, 3, 2, 1});
  Stride::Permutation perm(symbolic);
  EXPECT_EQ(perm.size(), 5);
  EXPECT_EQ(perm[0], 3);
  EXPECT_EQ(perm[1], 3);
  EXPECT_EQ(perm[2], 2);
  EXPECT_EQ(perm[3], 1);
  EXPECT_EQ(perm[4], 0);
}

TEST_F(StridePermutationTest, IsCanonical) {
  Stride::Permutation canonical({0, 1, 2, 3});
  EXPECT_TRUE(canonical.is_canonical());

  Stride::Permutation non_canonical({1, 0, 2, 3});
  EXPECT_FALSE(non_canonical.is_canonical());
}

TEST_F(StridePermutationTest, IsDegenerate) {
  Stride::Permutation valid({0, 1, 2, 3});
  EXPECT_FALSE(valid.is_degenerate());

  Stride::Permutation degenerate_one({0, 1, 1, 2});
  EXPECT_TRUE(degenerate_one.is_degenerate());

  Stride::Permutation degenerate_two({0, 1, 1, 3});
  EXPECT_TRUE(degenerate_two.is_degenerate());
}

TEST_F(StridePermutationTest, IsSanitised) {
  Stride::Permutation sanitised({0, 1, 2, 3});
  EXPECT_TRUE(sanitised.is_sanitised());

  Stride::Permutation invalid_min({1, 2, 3, 4});
  EXPECT_FALSE(invalid_min.is_sanitised());

  Stride::Permutation invalid_max({0, 1, 2, 5});
  EXPECT_FALSE(invalid_max.is_sanitised());

  Stride::Permutation duplicate({0, 1, 1, 3});
  EXPECT_FALSE(duplicate.is_sanitised());

  Stride::Permutation invalid_present({0, 1, 2, -1});
  EXPECT_FALSE(invalid_present.is_sanitised());
}

TEST_F(StridePermutationTest, SanitiseNoChange) {
  Stride::Permutation already_sanitised({3, 1, 2, 0});
  Stride::Permutation now_sanitised = already_sanitised.sanitised();
  EXPECT_TRUE(now_sanitised.is_sanitised());
  EXPECT_TRUE(now_sanitised == already_sanitised);
}

TEST_F(StridePermutationTest, SanitiseInvalidMax) {
  Stride::Permutation invalid_max({4, 1, 2, 0});
  invalid_max.sanitise();
  EXPECT_TRUE(invalid_max.is_sanitised());
  EXPECT_EQ(invalid_max[0], 3);
  EXPECT_EQ(invalid_max[1], 1);
  EXPECT_EQ(invalid_max[2], 2);
  EXPECT_EQ(invalid_max[3], 0);
}

TEST_F(StridePermutationTest, SanitiseDuplicate) {
  Stride::Permutation duplicate({0, 1, 1, 3});
  duplicate.sanitise();
  EXPECT_TRUE(duplicate.is_sanitised());
  EXPECT_EQ(duplicate[0], 0);
  EXPECT_EQ(duplicate[1], 1);
  EXPECT_EQ(duplicate[2], 2);
  EXPECT_EQ(duplicate[3], 3);
}

TEST_F(StridePermutationTest, SanitiseInvalidPresent) {
  Stride::Permutation invalid_present({2, 0, -1, 1});
  EXPECT_THROW(invalid_present.sanitise(), MR::Exception);
}

// TODO Sort out difference between is_sanitised() and valid() for Permutation
TEST_F(StridePermutationTest, Valid) {
  Stride::Permutation valid({0, 1, 2, 3});
  EXPECT_TRUE(valid.valid());

  Stride::Permutation negative({-1, 0, 1, 2});
  EXPECT_FALSE(negative.valid());
}

TEST_F(StridePermutationTest, Head) {
  Stride::Permutation perm({3, 2, 1, 0});
  auto head = perm.head(2);
  EXPECT_EQ(head.size(), 2);
  EXPECT_EQ(head[0], 3);
  EXPECT_EQ(head[1], 2);
  EXPECT_TRUE(head.valid());
  EXPECT_FALSE(head.is_sanitised());

  EXPECT_DEATH(perm.head(5), "Assertion `num_axes <= size\\(\\)' failed.");
}

TEST_F(StridePermutationTest, AxisRange) {
  auto perm = Stride::Permutation::axis_range(2, 5);
  EXPECT_EQ(perm.size(), 3);
  EXPECT_EQ(perm[0], 2);
  EXPECT_EQ(perm[1], 3);
  EXPECT_EQ(perm[2], 4);
  EXPECT_TRUE(perm.valid());
  EXPECT_FALSE(perm.is_sanitised());
}

TEST_F(StridePermutationTest, Canonical) {
  auto perm = Stride::Permutation::canonical(5);
  EXPECT_EQ(perm.size(), 5);
  EXPECT_TRUE(perm.valid());
  EXPECT_TRUE(perm.is_sanitised());
  EXPECT_TRUE(perm.is_canonical());
  for (size_t i = 0; i < 5; ++i)
    EXPECT_EQ(perm[i], i);
}

TEST_F(StridePermutationTest, ContiguousAlongAxis) {
  auto perm = Stride::Permutation::contiguous_along_axis(4, 2);
  EXPECT_EQ(perm.size(), 4);
  EXPECT_EQ(perm[0], 1);
  EXPECT_EQ(perm[1], 1);
  EXPECT_EQ(perm[2], 0);
  EXPECT_EQ(perm[3], 1);
  EXPECT_TRUE(perm.valid());
  EXPECT_FALSE(perm.is_sanitised());
}

TEST_F(StridePermutationTest, ContiguousAlongSpatialAxes) {
  auto perm = Stride::Permutation::contiguous_along_spatial_axes(5);
  EXPECT_EQ(perm.size(), 5);
  EXPECT_EQ(perm[0], 0);
  EXPECT_EQ(perm[1], 0);
  EXPECT_EQ(perm[2], 0);
  EXPECT_EQ(perm[3], 1);
  EXPECT_EQ(perm[4], 1);
  EXPECT_TRUE(perm.valid());
  EXPECT_FALSE(perm.is_sanitised());
}

// =============================================================================
// Symbolic class tests (New implementation)
// =============================================================================

class StrideSymbolicTest : public ::testing::Test {};

TEST_F(StrideSymbolicTest, ConstructFromVector) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  EXPECT_EQ(symbolic.size(), 4);
  EXPECT_TRUE(symbolic.valid());
  EXPECT_TRUE(symbolic.is_sanitised());
  EXPECT_TRUE(symbolic.is_canonical());
}

TEST_F(StrideSymbolicTest, ConstructFromActualNoTies) {
  Stride::Actual actual({-1, -10, 100, 1000});
  Stride::Symbolic symbolic(actual);
  EXPECT_EQ(symbolic.size(), 4);
  EXPECT_EQ(symbolic[0], -1);
  EXPECT_EQ(symbolic[1], -2);
  EXPECT_EQ(symbolic[2], 3);
  EXPECT_EQ(symbolic[3], 4);
  EXPECT_FALSE(actual.is_degenerate());
  EXPECT_FALSE(symbolic.is_degenerate());
}

TEST_F(StrideSymbolicTest, ConstructFromActualWithTies) {
  Stride::Actual actual({-1, -10, 10, 100});
  Stride::Symbolic symbolic(actual);
  EXPECT_EQ(symbolic.size(), 4);
  EXPECT_EQ(symbolic[0], -1);
  EXPECT_EQ(symbolic[1], -2);
  EXPECT_EQ(symbolic[2], 2);
  EXPECT_EQ(symbolic[3], 4);
  EXPECT_TRUE(actual.is_degenerate());
  EXPECT_TRUE(symbolic.is_degenerate());
}

TEST_F(StrideSymbolicTest, IsCanonical) {
  Stride::Symbolic canonical({1, 2, 3, 4});
  EXPECT_TRUE(canonical.is_canonical());

  Stride::Symbolic permuted({3, 1, 2, 4});
  EXPECT_FALSE(permuted.is_canonical());

  Stride::Symbolic negative({-1, 2, 3, 4});
  EXPECT_FALSE(negative.is_canonical());
}

// TODO Is is_degenerate() really required, or is the valid() / is_sanitised() adequate?
TEST_F(StrideSymbolicTest, IsDegenerate) {
  Stride::Symbolic valid({1, 2, 3, 4});
  EXPECT_FALSE(valid.is_degenerate());

  Stride::Symbolic::vector_type degenerate_vec{1, 2, 2, 4};
  auto degenerate = Stride::Symbolic(degenerate_vec);
  EXPECT_TRUE(degenerate.is_degenerate());
  degenerate.sanitise();
  EXPECT_FALSE(degenerate.is_degenerate());
}

TEST_F(StrideSymbolicTest, IsSanitised) {
  Stride::Symbolic sanitised({1, 2, 3, 4});
  EXPECT_TRUE(sanitised.is_sanitised());

  Stride::Symbolic invalid_min({0, 2, 3, 4});
  EXPECT_FALSE(invalid_min.is_sanitised());

  Stride::Symbolic invalid_max({1, 2, 3, 5});
  EXPECT_FALSE(invalid_max.is_sanitised());

  Stride::Symbolic duplicate({1, 2, 2, 4});
  EXPECT_FALSE(duplicate.is_sanitised());
}

TEST_F(StrideSymbolicTest, Valid) {
  Stride::Symbolic valid({1, 2, 3, 4});
  EXPECT_TRUE(valid.valid());

  Stride::Symbolic::vector_type invalid_vec{1, 0, 3, 4};
  Stride::Symbolic invalid(invalid_vec);
  EXPECT_FALSE(invalid.valid());

  invalid.sanitise();
  EXPECT_TRUE(invalid.valid());
}

TEST_F(StrideSymbolicTest, Flip) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  symbolic.flip(1);
  EXPECT_EQ(symbolic[0], 1);
  EXPECT_EQ(symbolic[1], -2);
  EXPECT_EQ(symbolic[2], 3);
  EXPECT_EQ(symbolic[3], 4);

  EXPECT_DEATH(symbolic.flip(-1), "Assertion `axis >= 0 && axis < size\\(\\)' failed.");
  EXPECT_DEATH(symbolic.flip(4), "Assertion `axis >= 0 && axis < size\\(\\)' failed.");
}

TEST_F(StrideSymbolicTest, Head) {
  Stride::Symbolic symbolic({4, 3, 2, 1});
  auto head = symbolic.head(2);
  EXPECT_EQ(head.size(), 2);
  EXPECT_EQ(head[0], 4);
  EXPECT_EQ(head[1], 3);

  EXPECT_DEATH(symbolic.head(5), "Assertion `num_axes <= size\\(\\)' failed.");
}

TEST_F(StrideSymbolicTest, Block) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  auto block = symbolic.block(1, 3);
  EXPECT_EQ(block.size(), 2);
  EXPECT_EQ(block[0], 2);
  EXPECT_EQ(block[1], 3);

  auto tail = symbolic.block(1);
  EXPECT_EQ(tail.size(), 3);
  EXPECT_EQ(tail[0], 2);
  EXPECT_EQ(tail[1], 3);
  EXPECT_EQ(tail[2], 4);

  EXPECT_DEATH(symbolic.block(-1), "Assertion `from_axis >= 0' failed.");
  EXPECT_DEATH(symbolic.block(5), "Assertion `from_axis < size\\(\\)' failed.");
  EXPECT_DEATH(symbolic.block(2, 2), "Assertion `from_axis < to_axis' failed.");
  EXPECT_DEATH(symbolic.block(0, 5), "Assertion `to_axis < size\\(\\)' failed.");
}

TEST_F(StrideSymbolicTest, Resize) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  symbolic.resize(6);
  EXPECT_EQ(symbolic.size(), 6);
  EXPECT_EQ(symbolic[0], 1);
  EXPECT_EQ(symbolic[1], 2);
  EXPECT_EQ(symbolic[2], 3);
  EXPECT_EQ(symbolic[3], 4);
  EXPECT_EQ(symbolic[4], 0);
  EXPECT_EQ(symbolic[5], 0);

  symbolic.resize(3);
  EXPECT_EQ(symbolic.size(), 3);
  EXPECT_EQ(symbolic[0], 1);
  EXPECT_EQ(symbolic[1], 2);
  EXPECT_EQ(symbolic[2], 3);
}

TEST_F(StrideSymbolicTest, Resized) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  auto resized = symbolic.resized(2);
  EXPECT_EQ(resized.size(), 2);
  EXPECT_EQ(symbolic.size(), 4); // Original unchanged
}

TEST_F(StrideSymbolicTest, Conform) {
  // Examples pulled from MR::Stride::Legacy inline documentation:
  // - \c current: [ 1 2 3 4 ], \c desired: [ 0 0 0 1 ] => [ 2 3 4 1 ]
  // - \c current: [ 3 -2 4 1 ], \c desired: [ 0 0 0 1 ] => [ 3 -2 4 1 ]
  // - \c current: [ -2 4 -3 1 ], \c desired: [ 1 2 3 0 ] => [ 1 2 3 4 ]
  // - \c current: [ -1 2 -3 4 ], \c desired: [ 1 2 3 0 ] => [ -1 2 -3 4 ]
  {
    Stride::Symbolic symbolic({1, 2, 3, 4});
    Stride::Symbolic desired({0, 0, 0, 1});
    symbolic.conform(desired);
    EXPECT_EQ(symbolic[0], 2);
    EXPECT_EQ(symbolic[1], 3);
    EXPECT_EQ(symbolic[2], 4);
    EXPECT_EQ(symbolic[3], 1);
  }
  {
    Stride::Symbolic symbolic({3, -2, 4, 1});
    Stride::Symbolic desired({0, 0, 0, 1});
    symbolic.conform(desired);
    EXPECT_EQ(symbolic[0], 3);
    EXPECT_EQ(symbolic[1], -2);
    EXPECT_EQ(symbolic[2], 4);
    EXPECT_EQ(symbolic[3], 1);
  }
  {
    Stride::Symbolic symbolic({-2, 4, 3, 1});
    Stride::Symbolic desired({1, 2, 3, 0});
    symbolic.conform(desired);
    EXPECT_EQ(symbolic[0], 1);
    EXPECT_EQ(symbolic[1], 2);
    EXPECT_EQ(symbolic[2], 3);
    EXPECT_EQ(symbolic[3], 4);
  }
  // TODO Suspect this is different:
  //   the claim in MR::Stride::Legacy seems to refer to a permutation rather than a symbolic conformation
  // TODO There might be a way to disambiguate this at the command-line:
  //   use a custom parser that tracks whether a positive symbolic stride is implicit or explicit;
  //   if implicit, treat as an axis permutation;
  //   if explicit, force that symbolic stride
  {
    Stride::Symbolic symbolic({-1, 2, -3, 4});
    Stride::Symbolic desired({1, 2, 3, 0});
    symbolic.conform(desired);
    EXPECT_EQ(symbolic[0], -1);
    EXPECT_EQ(symbolic[1], 2);
    EXPECT_EQ(symbolic[2], -3);
    EXPECT_EQ(symbolic[3], 4);
  }
  {
    Stride::Symbolic symbolic({1, 2, 3, 4});
    Stride::Symbolic desired({0, 0, 0});
    EXPECT_DEATH(symbolic.conform(desired), "Assertion `in.size\\(\\) == size\\(\\)' failed.");
  }
}

TEST_F(StrideSymbolicTest, Conformed) {
  Stride::Symbolic symbolic({2, 1, 3, 4});
  Stride::Symbolic target({1, 2, 3, 4});
  auto conformed = symbolic.conformed(target);
  EXPECT_TRUE(conformed.is_sanitised());
  // Original unchanged
  EXPECT_EQ(symbolic[0], 2);
  EXPECT_EQ(symbolic[1], 1);
}

TEST_F(StrideSymbolicTest, Reorder) {
  {
    Stride::Symbolic symbolic({1, 2, 3, 4});
    Stride::Permutation perm({1, 0, 2, 3});
    symbolic.reorder(perm);
    EXPECT_TRUE(symbolic.is_sanitised());
    EXPECT_EQ(symbolic[0], 2);
    EXPECT_EQ(symbolic[1], 1);
    EXPECT_EQ(symbolic[2], 3);
    EXPECT_EQ(symbolic[3], 4);
  }
  {
    Stride::Symbolic symbolic({3, 2, 1, 4});
    Stride::Permutation perm({1, 1, 1, 0});
    symbolic.reorder(perm);
    EXPECT_TRUE(symbolic.is_sanitised());
    EXPECT_EQ(symbolic[0], 4);
    EXPECT_EQ(symbolic[1], 3);
    EXPECT_EQ(symbolic[2], 2);
    EXPECT_EQ(symbolic[3], 1);
  }
  {
    Stride::Symbolic symbolic({3, 2, 1, 4});
    Stride::Permutation perm({0, 1, 2});
    EXPECT_DEATH(symbolic.reorder(perm), "Assertion `permutation.size\\(\\) == size\\(\\)' failed.");
  }
}

TEST_F(StrideSymbolicTest, Reordered) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  Stride::Permutation perm({1, 0, 2, 3});
  auto reordered = symbolic.reordered(perm);
  EXPECT_TRUE(reordered.valid());
  // Original unchanged
  EXPECT_EQ(symbolic[0], 1);
  EXPECT_EQ(symbolic[1], 2);
}

TEST_F(StrideSymbolicTest, Canonical) {
  auto symbolic = Stride::Symbolic::canonical(5);
  EXPECT_EQ(symbolic.size(), 5);
  EXPECT_TRUE(symbolic.is_canonical());
  for (size_t i = 0; i < 5; ++i)
    EXPECT_EQ(symbolic[i], i + 1);
}

TEST_F(StrideSymbolicTest, NegativeStrides) {
  Stride::Symbolic symbolic({-1, -2, -3, -4});
  EXPECT_TRUE(symbolic.is_sanitised());
  EXPECT_EQ(symbolic[0], -1);
  EXPECT_EQ(symbolic[1], -2);
  EXPECT_EQ(symbolic[2], -3);
  EXPECT_EQ(symbolic[3], -4);
}

// =============================================================================
// Actual class tests (New implementation)
// =============================================================================

class StrideActualTest : public ::testing::Test {};

TEST_F(StrideActualTest, ConstructFromVector) {
  Stride::Actual actual({1, 10, 100, 1000});
  EXPECT_EQ(actual.size(), 4);
  EXPECT_EQ(actual[0], 1);
  EXPECT_EQ(actual[1], 10);
  EXPECT_EQ(actual[2], 100);
  EXPECT_EQ(actual[3], 1000);
}

TEST_F(StrideActualTest, ConstructFromSymbolicAndSizes) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  std::vector<size_t> sizes{10, 20, 30, 40};
  Stride::Actual actual(symbolic, sizes);
  EXPECT_EQ(actual.size(), 4);
  EXPECT_EQ(actual[0], 1);
  EXPECT_EQ(actual[1], 10);
  EXPECT_EQ(actual[2], 200);
  EXPECT_EQ(actual[3], 6000);

  sizes.resize(3);
  EXPECT_DEATH(Stride::Actual(symbolic, sizes), "Assertion `symbolic.size\\(\\) == sizes.size\\(\\)' failed.");
}

TEST_F(StrideActualTest, ConstructFromMockHeader) {
  MockHeader header({10, 20, 30}, {1, 2, 3});
  Stride::Actual actual(header);
  EXPECT_EQ(actual.size(), 3);
  EXPECT_EQ(actual[0], 1);
  EXPECT_EQ(actual[1], 10);
  EXPECT_EQ(actual[2], 200);
}

TEST_F(StrideActualTest, IsCanonical) {
  Stride::Actual canonical({1, 10, 100, 1000});
  EXPECT_TRUE(canonical.is_canonical());

  // Perfectly sane result if the third axis were of size 1
  Stride::Actual duplicate({1, 10, 100, 100});
  EXPECT_TRUE(duplicate.is_canonical());

  Stride::Actual permuted({100, 1, 10, 1000});
  EXPECT_FALSE(permuted.is_canonical());

  Stride::Actual negative({-1, 10, 100, 1000});
  EXPECT_FALSE(negative.is_canonical());
}

TEST_F(StrideActualTest, Valid) {
  Stride::Actual valid({1, 10, 100, 1000});
  EXPECT_TRUE(valid.valid());

  Stride::Actual with_zero({0, 10, 100, 1000});
  EXPECT_FALSE(with_zero.valid());
}

TEST_F(StrideActualTest, ToSymbolicCanonical) {
  Stride::Actual canonical({1, 10, 100, 1000});
  Stride::Symbolic symbolic = canonical.symbolic();
  EXPECT_EQ(symbolic.size(), 4);
  EXPECT_TRUE(symbolic.is_canonical());
}
TEST_F(StrideActualTest, ToSymbolicNegatives) {
  Stride::Actual negatives({-1, -10, 100, 1000});
  Stride::Symbolic symbolic = negatives.symbolic();
  EXPECT_EQ(symbolic.size(), 4);
  EXPECT_EQ(symbolic[0], -1);
  EXPECT_EQ(symbolic[1], -2);
  EXPECT_EQ(symbolic[2], 3);
  EXPECT_EQ(symbolic[3], 4);
}

TEST_F(StrideActualTest, ToSymbolicTies) {
  Stride::Actual tie({10, 1, 1, 100});
  Stride::Symbolic symbolic = tie.symbolic();
  EXPECT_EQ(symbolic.size(), 4);
  EXPECT_EQ(symbolic[0], 3);
  EXPECT_EQ(symbolic[1], 1);
  EXPECT_EQ(symbolic[2], 1);
  EXPECT_EQ(symbolic[3], 4);
  EXPECT_TRUE(symbolic.valid());
  EXPECT_TRUE(symbolic.is_degenerate());
}
TEST_F(StrideActualTest, ToSymbolicNoncanonical) {
  Stride::Actual permuted({10, 100, 1000, 1});
  Stride::Symbolic symbolic = permuted.symbolic();
  EXPECT_EQ(symbolic.size(), 4);
  EXPECT_EQ(symbolic[0], 2);
  EXPECT_EQ(symbolic[1], 3);
  EXPECT_EQ(symbolic[2], 4);
  EXPECT_EQ(symbolic[3], 1);
}

TEST_F(StrideActualTest, Match) {
  MockHeader header1({10, 20, 30}, {1, 10, 200});
  MockHeader header2({10, 20, 30}, {1, 10, 200});
  MockHeader header3({10, 20, 30}, {1, 600, 30});

  Stride::Actual actual(header1);
  EXPECT_TRUE(actual.match(header2));
  EXPECT_FALSE(actual.match(header3));
}

// =============================================================================
// Offset function tests (New implementation)
// =============================================================================

class StrideOffsetTest : public ::testing::Test {};

TEST_F(StrideOffsetTest, PositiveStrides) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  auto offset = Stride::offset(header);
  EXPECT_EQ(offset, 0);
}

TEST_F(StrideOffsetTest, NegativeStrides) {
  MockHeader header({10, 20, 30}, {-1, -10, -200});
  auto offset = Stride::offset(header);
  EXPECT_EQ(offset, 9 + 190 + 5800);
}

TEST_F(StrideOffsetTest, MixedStrides) {
  MockHeader header({10, 20, 30}, {1, -10, 200});
  auto offset = Stride::offset(header);
  EXPECT_EQ(offset, 190);
}

TEST_F(StrideOffsetTest, WithActualStrides) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  Stride::Actual actual(header);
  auto offset = Stride::offset(actual, header);
  EXPECT_EQ(offset, 0);
}

// =============================================================================
// Legacy namespace tests
// =============================================================================

class StrideLegacyOrderTest : public ::testing::Test {};

TEST_F(StrideLegacyOrderTest, OrderCanonical) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  auto order = Stride::Legacy::order(header);
  EXPECT_EQ(order.size(), 3);
  EXPECT_EQ(order[0], 0);
  EXPECT_EQ(order[1], 1);
  EXPECT_EQ(order[2], 2);
}

TEST_F(StrideLegacyOrderTest, OrderNonCanonical) {
  MockHeader header({10, 20, 30}, {600, 1, 20});
  auto order = Stride::Legacy::order(header);
  EXPECT_EQ(order.size(), 3);
  EXPECT_EQ(order[0], 1);
  EXPECT_EQ(order[1], 2);
  EXPECT_EQ(order[2], 0);
}

TEST_F(StrideLegacyOrderTest, OrderFromList) {
  Stride::Legacy::List strides{3, 1, 2};
  auto order = Stride::Legacy::order(strides);
  EXPECT_EQ(order.size(), 3);
  EXPECT_EQ(order[0], 1);
  EXPECT_EQ(order[1], 2);
  EXPECT_EQ(order[2], 0);
}

TEST_F(StrideLegacyOrderTest, OrderWithRange) {
  MockHeader header({10, 20, 30, 40}, {1, 10, 200, 6000});
  auto order = Stride::Legacy::order(header, 1, 3);
  EXPECT_EQ(order.size(), 2);
  EXPECT_EQ(order[0], 1);
  EXPECT_EQ(order[1], 2);
}

class StrideLegacySanitiseTest : public ::testing::Test {};

TEST_F(StrideLegacySanitiseTest, SanitiseDuplicates) {
  MockHeader header({10, 20, 30}, {1, 1, 2});
  Stride::Legacy::sanitise(header);
  EXPECT_NE(header.stride(0), header.stride(1));
  EXPECT_NE(header.stride(0), header.stride(2));
  EXPECT_NE(header.stride(1), header.stride(2));
}

TEST_F(StrideLegacySanitiseTest, SanitiseZeros) {
  MockHeader header({10, 20, 30}, {1, 0, 2});
  Stride::Legacy::sanitise(header);
  EXPECT_NE(header.stride(0), 0);
  EXPECT_NE(header.stride(1), 0);
  EXPECT_NE(header.stride(2), 0);
  EXPECT_NE(header.stride(0), header.stride(1));
  EXPECT_NE(header.stride(0), header.stride(2));
  EXPECT_NE(header.stride(1), header.stride(2));
}

TEST_F(StrideLegacySanitiseTest, SanitiseWithList) {
  Stride::Legacy::List strides{1, 0, 2};
  MockHeader header({10, 20, 30}, {1, 10, 200});
  Stride::Legacy::sanitise(strides, header);
  EXPECT_NE(strides[0], 0);
  EXPECT_NE(strides[1], 0);
  EXPECT_NE(strides[2], 0);
  EXPECT_NE(header.stride(0), header.stride(1));
  EXPECT_NE(header.stride(0), header.stride(2));
  EXPECT_NE(header.stride(1), header.stride(2));
}

TEST_F(StrideLegacySanitiseTest, SanitiseUnitySizeAxis) {
  MockHeader header({10, 1, 30}, {1, 2, 3});
  Stride::Legacy::sanitise(header);
  // Axis with size 1 should have stride set to 0 initially, then sanitised
  EXPECT_NE(header.stride(0), 0);
  EXPECT_EQ(header.stride(1), 0);
  EXPECT_NE(header.stride(2), 0);
}

class StrideLegacyActualiseTest : public ::testing::Test {};

TEST_F(StrideLegacyActualiseTest, ActualiseSymbolic) {
  MockHeader header({10, 20, 30}, {1, 2, 3});
  Stride::Legacy::actualise(header);
  EXPECT_EQ(header.stride(0), 1);
  EXPECT_EQ(header.stride(1), 10);
  EXPECT_EQ(header.stride(2), 200);
}

TEST_F(StrideLegacyActualiseTest, ActualiseNegative) {
  MockHeader header({10, 20, 30}, {-1, -2, -3});
  Stride::Legacy::actualise(header);
  EXPECT_EQ(header.stride(0), -1);
  EXPECT_EQ(header.stride(1), -10);
  EXPECT_EQ(header.stride(2), -200);
}

TEST_F(StrideLegacyActualiseTest, ActualiseWithList) {
  Stride::Legacy::List strides{3, 1, 2};
  MockHeader header({10, 20, 30}, {1, 10, 200});
  Stride::Legacy::actualise(strides, header);
  EXPECT_EQ(strides[0], 600);
  EXPECT_EQ(strides[1], 1);
  EXPECT_EQ(strides[2], 20);
}

TEST_F(StrideLegacyActualiseTest, GetActual) {
  MockHeader header({10, 20, 30}, {1, 2, 3});
  auto actual = Stride::Legacy::get_actual(header);
  EXPECT_EQ(actual[0], 1);
  EXPECT_EQ(actual[1], 10);
  EXPECT_EQ(actual[2], 200);
}

class StrideLegacySymboliseTest : public ::testing::Test {};

TEST_F(StrideLegacySymboliseTest, SymboliseActual) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  Stride::Legacy::symbolise(header);
  EXPECT_EQ(header.stride(0), 1);
  EXPECT_EQ(header.stride(1), 2);
  EXPECT_EQ(header.stride(2), 3);
}

TEST_F(StrideLegacySymboliseTest, SymboliseNegative) {
  MockHeader header({10, 20, 30}, {-1, -10, -200});
  Stride::Legacy::symbolise(header);
  EXPECT_EQ(header.stride(0), -1);
  EXPECT_EQ(header.stride(1), -2);
  EXPECT_EQ(header.stride(2), -3);
}

TEST_F(StrideLegacySymboliseTest, SymboliseList) {
  Stride::Legacy::List strides{1, 10, 200};
  Stride::Legacy::symbolise(strides);
  EXPECT_EQ(strides[0], 1);
  EXPECT_EQ(strides[1], 2);
  EXPECT_EQ(strides[2], 3);
}

TEST_F(StrideLegacySymboliseTest, GetSymbolic) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  auto symbolic = Stride::Legacy::get_symbolic(header);
  EXPECT_EQ(symbolic[0], 1);
  EXPECT_EQ(symbolic[1], 2);
  EXPECT_EQ(symbolic[2], 3);
}

class StrideLegacyOffsetTest : public ::testing::Test {};

TEST_F(StrideLegacyOffsetTest, OffsetPositiveStrides) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  auto offset = Stride::Legacy::offset(header);
  EXPECT_EQ(offset, 0);
}

TEST_F(StrideLegacyOffsetTest, OffsetNegativeStrides) {
  MockHeader header({10, 20, 30}, {-1, -10, -200});
  auto offset = Stride::Legacy::offset(header);
  EXPECT_EQ(offset, 9 + 190 + 5800);
}

TEST_F(StrideLegacyOffsetTest, OffsetMixedStrides) {
  MockHeader header({10, 20, 30}, {1, -10, 200});
  auto offset = Stride::Legacy::offset(header);
  EXPECT_EQ(offset, 190);
}

TEST_F(StrideLegacyOffsetTest, OffsetWithList) {
  Stride::Legacy::List strides{-1, -10, -200};
  MockHeader header({10, 20, 30}, {1, 10, 200});
  auto offset = Stride::Legacy::offset(strides, header);
  EXPECT_EQ(offset, 9 + 190 + 5800);
}

class StrideLegacyGetNearestMatchTest : public ::testing::Test {};

// From MR::Stride::
// - \c current: [ 1 2 3 4 ], \c desired: [ 0 0 0 1 ] => [ 2 3 4 1 ]
// - \c current: [ 3 -2 4 1 ], \c desired: [ 0 0 0 1 ] => [ 3 -2 4 1 ]
// - \c current: [ -2 4 -3 1 ], \c desired: [ 1 2 3 0 ] => [ 1 2 3 4 ]
// - \c current: [ -1 2 -3 4 ], \c desired: [ 1 2 3 0 ] => [ -1 2 -3 4 ]

TEST_F(StrideLegacyGetNearestMatchTest, All) {
  {
    MockHeader header({10, 20, 30, 40}, {1, 2, 3, 4});
    Stride::Legacy::List desired{0, 0, 0, 1};
    auto result = Stride::Legacy::get_nearest_match(header, desired);
    EXPECT_EQ(result.size(), 4);
    EXPECT_EQ(result[0], 2);
    EXPECT_EQ(result[1], 3);
    EXPECT_EQ(result[2], 4);
    EXPECT_EQ(result[3], 1);
  }
  {
    MockHeader header({10, 20, 30, 40}, {3, -2, 4, 1});
    Stride::Legacy::List desired{0, 0, 0, 1};
    auto result = Stride::Legacy::get_nearest_match(header, desired);
    EXPECT_EQ(result.size(), 4);
    EXPECT_EQ(result[0], 3);
    EXPECT_EQ(result[1], -2);
    EXPECT_EQ(result[2], 4);
    EXPECT_EQ(result[3], 1);
  }
  {
    MockHeader header({10, 20, 30, 40}, {-2, 4, -3, 1});
    Stride::Legacy::List desired{1, 2, 3, 0};
    auto result = Stride::Legacy::get_nearest_match(header, desired);
    EXPECT_EQ(result.size(), 4);
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[1], 2);
    EXPECT_EQ(result[2], 3);
    EXPECT_EQ(result[3], 4);
  }
  {
    MockHeader header({10, 20, 30, 40}, {-1, 2, -3, 4});
    Stride::Legacy::List desired{1, 2, 3, 0};
    auto result = Stride::Legacy::get_nearest_match(header, desired);
    EXPECT_EQ(result.size(), 4);
    EXPECT_EQ(result[0], -1);
    EXPECT_EQ(result[1], 2);
    EXPECT_EQ(result[2], -3);
    EXPECT_EQ(result[3], 4);
  }
}

class StrideLegacyContiguousTest : public ::testing::Test {};

TEST_F(StrideLegacyContiguousTest, ContiguousAlongAxis) {
  auto strides = Stride::Legacy::contiguous_along_axis(3);
  EXPECT_EQ(strides.size(), 4);
  EXPECT_EQ(strides[0], 0);
  EXPECT_EQ(strides[1], 0);
  EXPECT_EQ(strides[2], 0);
  EXPECT_EQ(strides[3], 1);
}

TEST_F(StrideLegacyContiguousTest, ContiguousAlongAxisWithHeader) {
  MockHeader header({10, 20, 30, 40}, {1, 2, 3, 4});
  auto strides = Stride::Legacy::contiguous_along_axis(3, header);
  EXPECT_EQ(strides.size(), 4);
  EXPECT_EQ(strides[0], 2);
  EXPECT_EQ(strides[1], 3);
  EXPECT_EQ(strides[2], 4);
  EXPECT_EQ(strides[3], 1);
}

TEST_F(StrideLegacyContiguousTest, ContiguousAlongSpatialAxes) {
  MockHeader header({10, 20, 30, 40, 50}, {5, 4, 3, 2, 1});
  auto strides = Stride::Legacy::contiguous_along_spatial_axes(header);
  EXPECT_EQ(strides.size(), 5);
  EXPECT_EQ(strides[0], 5);
  EXPECT_EQ(strides[1], 4);
  EXPECT_EQ(strides[2], 3);
  EXPECT_EQ(strides[3], 0);
  EXPECT_EQ(strides[4], 0);
}

// =============================================================================
// Integration tests: New vs Legacy
// =============================================================================

class StrideIntegrationTest : public ::testing::Test {};

TEST_F(StrideIntegrationTest, SymbolicToActualConsistency) {
  // New implementation
  Stride::Symbolic symbolic_new({1, 2, 3, 4});
  std::vector<size_t> sizes{10, 20, 30, 40};
  Stride::Actual actual_new(symbolic_new, sizes);

  // Legacy implementation
  MockHeader header({10, 20, 30, 40}, {1, 2, 3, 4});
  Stride::Legacy::actualise(header);

  // Results should match
  EXPECT_EQ(actual_new[0], header.stride(0));
  EXPECT_EQ(actual_new[1], header.stride(1));
  EXPECT_EQ(actual_new[2], header.stride(2));
  EXPECT_EQ(actual_new[3], header.stride(3));
}

TEST_F(StrideIntegrationTest, ActualToSymbolicConsistency) {
  // New implementation
  Stride::Actual actual_new({1, 10, 200, 6000});
  Stride::Symbolic symbolic_new = actual_new.symbolic();

  // Legacy implementation
  MockHeader header({10, 20, 30, 40}, {1, 10, 200, 6000});
  Stride::Legacy::symbolise(header);

  // Results should match
  EXPECT_EQ(symbolic_new[0], header.stride(0));
  EXPECT_EQ(symbolic_new[1], header.stride(1));
  EXPECT_EQ(symbolic_new[2], header.stride(2));
  EXPECT_EQ(symbolic_new[3], header.stride(3));
}

TEST_F(StrideIntegrationTest, OrderConsistency) {
  // New implementation
  Stride::Symbolic symbolic_new({3, 1, 2, 4});
  Stride::Order order_new = symbolic_new.order();

  // Legacy implementation
  MockHeader header({10, 20, 30, 40}, {3, 1, 2, 4});
  auto order_legacy = Stride::Legacy::order(header);

  // Results should match
  EXPECT_EQ(order_new.size(), order_legacy.size());
  for (size_t i = 0; i < order_new.size(); ++i)
    EXPECT_EQ(order_new[i], order_legacy[i]);
}

TEST_F(StrideIntegrationTest, OffsetConsistency) {
  // New implementation
  MockHeader header1({10, 20, 30}, {-1, -10, -200});
  auto offset_new = Stride::offset(header1);

  // Legacy implementation
  MockHeader header2({10, 20, 30}, {-1, -10, -200});
  auto offset_legacy = Stride::Legacy::offset(header2);

  // Results should match
  EXPECT_EQ(offset_new, offset_legacy);
}

// New Stride::Symbolic::sanitise() is _not_ identical to Stride::Legacy::sanitise()
//   in the presence of ties:
//   the former disambiguates ties while preserving order relative to other axes;
//   the latter Demotes any ties to the end of the order
TEST_F(StrideIntegrationTest, SanitiseConsistency) {
  {
    Stride::Symbolic symbolic_new({-4, -9, -12, 16});
    symbolic_new.sanitise();
    MockHeader header({10, 20, 30, 40}, {-4, -9, -12, 16});
    Stride::Legacy::sanitise(header);
    auto result_legacy = Stride::Legacy::get_symbolic(header);
    EXPECT_EQ(symbolic_new.size(), result_legacy.size());
    for (size_t i = 0; i < symbolic_new.size(); ++i)
      EXPECT_EQ(symbolic_new[i], result_legacy[i]);
  }
  // {
  //   Stride::Symbolic symbolic_new({-4, -4, -12, 16});
  //   symbolic_new.sanitise();
  //   MockHeader header({10, 20, 30, 40}, {-4, -4, -12, 16});
  //   Stride::Legacy::sanitise(header);
  //   auto result_legacy = Stride::Legacy::get_symbolic(header);
  //   EXPECT_EQ(symbolic_new.size(), result_legacy.size());
  //   for (size_t i = 0; i < symbolic_new.size(); ++i)
  //     EXPECT_EQ(symbolic_new[i], result_legacy[i]);
  // }
}

TEST_F(StrideIntegrationTest, NearestMatchConsistency) {
  {
    Stride::Symbolic symbolic_new({1, 2, 3, 4});
    Stride::Symbolic desired_new({0, 0, 0, 1});
    symbolic_new.conform(desired_new);
    MockHeader header({10, 20, 30, 40}, {1, 2, 3, 4});
    std::vector<ssize_t> desired_legacy({0, 0, 0, 1});
    auto result_legacy = Stride::Legacy::get_nearest_match(header, desired_legacy);
    EXPECT_EQ(symbolic_new.size(), header.ndim());
    for (size_t i = 0; i < symbolic_new.size(); ++i)
      EXPECT_EQ(symbolic_new[i], result_legacy[i]);
  }
  {
    Stride::Symbolic symbolic_new({3, -2, 4, 1});
    Stride::Symbolic desired_new({0, 0, 0, 1});
    symbolic_new.conform(desired_new);
    MockHeader header({10, 20, 30, 40}, {3, -2, 4, 1});
    std::vector<ssize_t> desired_legacy({0, 0, 0, 1});
    auto result_legacy = Stride::Legacy::get_nearest_match(header, desired_legacy);
    EXPECT_EQ(symbolic_new.size(), header.ndim());
    for (size_t i = 0; i < symbolic_new.size(); ++i)
      EXPECT_EQ(symbolic_new[i], result_legacy[i]);
  }
  {
    Stride::Symbolic symbolic_new({-2, 4, 3, 1});
    Stride::Symbolic desired_new({1, 2, 3, 0});
    symbolic_new.conform(desired_new);
    MockHeader header({10, 20, 30, 40}, {-2, 4, 3, 1});
    std::vector<ssize_t> desired_legacy({1, 2, 3, 0});
    auto result_legacy = Stride::Legacy::get_nearest_match(header, desired_legacy);
    EXPECT_EQ(symbolic_new.size(), header.ndim());
    for (size_t i = 0; i < symbolic_new.size(); ++i)
      EXPECT_EQ(symbolic_new[i], result_legacy[i]);
  }
  {
    Stride::Symbolic symbolic_new({-1, 2, -3, 4});
    Stride::Symbolic desired_new({1, 2, 3, 0});
    symbolic_new.conform(desired_new);
    MockHeader header({10, 20, 30, 40}, {-1, 2, -3, 4});
    std::vector<ssize_t> desired_legacy({1, 2, 3, 0});
    auto result_legacy = Stride::Legacy::get_nearest_match(header, desired_legacy);
    EXPECT_EQ(symbolic_new.size(), header.ndim());
    for (size_t i = 0; i < symbolic_new.size(); ++i)
      EXPECT_EQ(symbolic_new[i], result_legacy[i]);
  }
}

// =============================================================================
// Invalid value tests
// =============================================================================

class StrideInvalidValueTest : public ::testing::Test {};

TEST_F(StrideInvalidValueTest, OrderInvalidValues) {
  // Empty order
  Stride::Order empty;
  EXPECT_FALSE(empty.valid());

  // Negative values
  Stride::Order negative(std::vector<ArrayIndex>{-1, 0, 1});
  EXPECT_FALSE(negative.valid());

  // Duplicate values
  Stride::Order duplicate(std::vector<ArrayIndex>{0, 0, 1});
  EXPECT_FALSE(duplicate.valid());
}

TEST_F(StrideInvalidValueTest, PermutationInvalidValues) {
  // Empty permutation
  Stride::Permutation empty;
  EXPECT_FALSE(empty.valid());

  // Negative values
  Stride::Permutation negative({-1, 0, 1});
  EXPECT_FALSE(negative.valid());
}

TEST_F(StrideInvalidValueTest, SymbolicInvalidValues) {
  // Empty symbolic
  Stride::Symbolic empty;
  EXPECT_FALSE(empty.valid());

  // Zero values
  Stride::Symbolic with_zero({0, 1, 2});
  EXPECT_FALSE(with_zero.valid());
}

TEST_F(StrideInvalidValueTest, ActualInvalidValues) {
  // Zero strides
  Stride::Actual with_zero({0, 10, 100});
  EXPECT_FALSE(with_zero.valid());
}

// =============================================================================
// Demonstration on documentation example
// =============================================================================

class StrideDemonstrationTest : public ::testing::Test {};

const Stride::Order true_order({3, 0, 1, 2});
const Stride::Permutation true_permutation({1, 2, 3, 0});
const Stride::Symbolic true_symbolic({-2, -3, 4, 1});
const Stride::Actual true_actual({-10, -320, 10240, 1});

TEST_F(StrideDemonstrationTest, ActualToSymbolic) {
  MockHeader header({32, 32, 20, 10}, {-10, -320, 10240, 1});
  Stride::Actual actual(header);
  {
    Stride::Symbolic symbolic(actual);
    EXPECT_EQ(symbolic[0], true_symbolic[0]);
    EXPECT_EQ(symbolic[1], true_symbolic[1]);
    EXPECT_EQ(symbolic[2], true_symbolic[2]);
    EXPECT_EQ(symbolic[3], true_symbolic[3]);
  }
  {
    auto symbolic = actual.symbolic();
    EXPECT_EQ(symbolic[0], true_symbolic[0]);
    EXPECT_EQ(symbolic[1], true_symbolic[1]);
    EXPECT_EQ(symbolic[2], true_symbolic[2]);
    EXPECT_EQ(symbolic[3], true_symbolic[3]);
  }
}

TEST_F(StrideDemonstrationTest, SymbolicToPermutation) {
  Stride::Symbolic symbolic({-2, -3, 4, 1});
  {
    Stride::Permutation permutation(symbolic);
    EXPECT_EQ(permutation[0], true_permutation[0]);
    EXPECT_EQ(permutation[1], true_permutation[1]);
    EXPECT_EQ(permutation[2], true_permutation[2]);
    EXPECT_EQ(permutation[3], true_permutation[3]);
  }
  {
    auto permutation(symbolic.permutation());
    EXPECT_EQ(permutation[0], true_permutation[0]);
    EXPECT_EQ(permutation[1], true_permutation[1]);
    EXPECT_EQ(permutation[2], true_permutation[2]);
    EXPECT_EQ(permutation[3], true_permutation[3]);
  }
}

TEST_F(StrideDemonstrationTest, PermutationToOrder) {
  Stride::Permutation permutation({1, 2, 3, 0});
  {
    Stride::Order order(permutation);
    EXPECT_EQ(order[0], true_order[0]);
    EXPECT_EQ(order[1], true_order[1]);
    EXPECT_EQ(order[2], true_order[2]);
    EXPECT_EQ(order[3], true_order[3]);
  }
  {
    auto order(permutation.order());
    EXPECT_EQ(order[0], true_order[0]);
    EXPECT_EQ(order[1], true_order[1]);
    EXPECT_EQ(order[2], true_order[2]);
    EXPECT_EQ(order[3], true_order[3]);
  }
}

TEST_F(StrideDemonstrationTest, OrderToPermutation) {
  Stride::Order order({3, 0, 1, 2});
  {
    Stride::Permutation permutation(order);
    EXPECT_EQ(permutation[0], true_permutation[0]);
    EXPECT_EQ(permutation[1], true_permutation[1]);
    EXPECT_EQ(permutation[2], true_permutation[2]);
    EXPECT_EQ(permutation[3], true_permutation[3]);
  }
  {
    auto permutation = order.permutation();
    EXPECT_EQ(permutation[0], true_permutation[0]);
    EXPECT_EQ(permutation[1], true_permutation[1]);
    EXPECT_EQ(permutation[2], true_permutation[2]);
    EXPECT_EQ(permutation[3], true_permutation[3]);
  }
}

TEST_F(StrideDemonstrationTest, PermutationToSymbolic) {
  Stride::Permutation permutation({1, 2, 3, 0});
  {
    Stride::Symbolic symbolic(permutation);
    // No way to know symbolic stride sign from permutation
    symbolic.flip(0);
    symbolic.flip(1);
    EXPECT_EQ(symbolic[0], true_symbolic[0]);
    EXPECT_EQ(symbolic[1], true_symbolic[1]);
    EXPECT_EQ(symbolic[2], true_symbolic[2]);
    EXPECT_EQ(symbolic[3], true_symbolic[3]);
  }
  {
    auto symbolic = permutation.symbolic();
    symbolic.flip(0);
    symbolic.flip(1);
    EXPECT_EQ(symbolic[0], true_symbolic[0]);
    EXPECT_EQ(symbolic[1], true_symbolic[1]);
    EXPECT_EQ(symbolic[2], true_symbolic[2]);
    EXPECT_EQ(symbolic[3], true_symbolic[3]);
  }
}

TEST_F(StrideDemonstrationTest, SymbolicToActual) {
  std::vector<size_t> sizes({32, 32, 20, 10});
  std::vector<ssize_t> strides({-2, -3, 4, 1});
  Stride::Symbolic symbolic({-2, -3, 4, 1});
  MockHeader header(sizes, strides);
  {
    Stride::Actual actual(header);
    EXPECT_EQ(actual[0], -10);
    EXPECT_EQ(actual[1], -320);
    EXPECT_EQ(actual[2], 10240);
    EXPECT_EQ(actual[3], 1);
  }
  {
    Stride::Actual actual(symbolic, sizes);
    EXPECT_EQ(actual[0], -10);
    EXPECT_EQ(actual[1], -320);
    EXPECT_EQ(actual[2], 10240);
    EXPECT_EQ(actual[3], 1);
  }
}