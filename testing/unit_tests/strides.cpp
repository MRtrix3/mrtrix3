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
  ssize_t &stride(size_t axis) { return strides_[axis]; }

private:
  std::vector<size_t> sizes_;
  std::vector<ssize_t> strides_;
};

// =============================================================================
// Order class tests (New implementation)
// =============================================================================

class OrderTest : public ::testing::Test {};

TEST_F(OrderTest, ConstructFromVector) {
  Stride::Order order(std::vector<ArrayIndex>{0, 1, 2, 3});
  EXPECT_EQ(order.size(), 4);
  EXPECT_EQ(order[0], 0);
  EXPECT_EQ(order[1], 1);
  EXPECT_EQ(order[2], 2);
  EXPECT_EQ(order[3], 3);
}

TEST_F(OrderTest, ConstructFromPermutation) {
  Stride::Permutation perm({0, 1, 2, 3});
  Stride::Order order(perm);
  EXPECT_EQ(order.size(), 4);
  EXPECT_TRUE(order.is_canonical());
}

TEST_F(OrderTest, IsCanonical) {
  Stride::Order canonical_order(std::vector<ArrayIndex>{0, 1, 2, 3});
  EXPECT_TRUE(canonical_order.is_canonical());

  Stride::Order non_canonical(std::vector<ArrayIndex>{3, 0, 1, 2});
  EXPECT_FALSE(non_canonical.is_canonical());
}

TEST_F(OrderTest, IsSanitised) {
  Stride::Order sanitised(std::vector<ArrayIndex>{0, 1, 2, 3});
  EXPECT_TRUE(sanitised.is_sanitised());

  Stride::Order invalid_min(std::vector<ArrayIndex>{1, 2, 3, 4});
  EXPECT_FALSE(invalid_min.is_sanitised());

  Stride::Order invalid_max(std::vector<ArrayIndex>{0, 1, 2, 2});
  EXPECT_FALSE(invalid_max.is_sanitised());
}

TEST_F(OrderTest, Valid) {
  Stride::Order valid_order(std::vector<ArrayIndex>{2, 0, 1, 3});
  EXPECT_TRUE(valid_order.valid());

  Stride::Order duplicate(std::vector<ArrayIndex>{0, 1, 1, 3});
  EXPECT_FALSE(duplicate.valid());

  Stride::Order negative(std::vector<ArrayIndex>{-1, 0, 1, 2});
  EXPECT_FALSE(negative.valid());
}

TEST_F(OrderTest, Sanitise) {
  Stride::Order order(std::vector<ArrayIndex>{3, 1, 2, 0});
  order.sanitise();
  EXPECT_TRUE(order.is_sanitised());
  EXPECT_EQ(order[0], 3);
  EXPECT_EQ(order[1], 0);
  EXPECT_EQ(order[2], 1);
  EXPECT_EQ(order[3], 2);
}

TEST_F(OrderTest, HeadAndTail) {
  Stride::Order order(std::vector<ArrayIndex>{3, 0, 1, 2});

  auto head = order.head(2);
  EXPECT_EQ(head.size(), 2);
  EXPECT_EQ(head[0], 3);
  EXPECT_EQ(head[1], 0);

  auto tail = order.tail(2);
  EXPECT_EQ(tail.size(), 2);
  EXPECT_EQ(tail[0], 1);
  EXPECT_EQ(tail[1], 2);
}

TEST_F(OrderTest, Subset) {
  Stride::Order order(std::vector<ArrayIndex>{3, 0, 1, 2});

  auto subset = order.subset(0, 2);
  EXPECT_EQ(subset.size(), 2);
  EXPECT_EQ(subset[0], 0);
  EXPECT_EQ(subset[1], 1);
}

TEST_F(OrderTest, Canonical) {
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

class PermutationTest : public ::testing::Test {};

TEST_F(PermutationTest, ConstructFromVector) {
  Stride::Permutation perm({0, 1, 2, 3});
  EXPECT_EQ(perm.size(), 4);
  EXPECT_TRUE(perm.is_canonical());
}

TEST_F(PermutationTest, ConstructFromOrder) {
  Stride::Order order(std::vector<ArrayIndex>{3, 0, 1, 2});
  Stride::Permutation perm(order);
  EXPECT_EQ(perm.size(), 4);
  EXPECT_EQ(perm[3], 0);
  EXPECT_EQ(perm[0], 1);
  EXPECT_EQ(perm[1], 2);
  EXPECT_EQ(perm[2], 3);
}

TEST_F(PermutationTest, ConstructFromSymbolic) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  Stride::Permutation perm(symbolic);
  EXPECT_EQ(perm.size(), 4);
  EXPECT_TRUE(perm.is_canonical());
}

TEST_F(PermutationTest, IsCanonical) {
  Stride::Permutation canonical({0, 1, 2, 3});
  EXPECT_TRUE(canonical.is_canonical());

  Stride::Permutation non_canonical({1, 0, 2, 3});
  EXPECT_FALSE(non_canonical.is_canonical());
}

TEST_F(PermutationTest, IsDegenerate) {
  Stride::Permutation valid({0, 1, 2, 3});
  EXPECT_FALSE(valid.is_degenerate());

  Stride::Permutation degenerate({0, 1, 1, 3});
  EXPECT_TRUE(degenerate.is_degenerate());
}

TEST_F(PermutationTest, IsSanitised) {
  Stride::Permutation sanitised({0, 1, 2, 3});
  EXPECT_TRUE(sanitised.is_sanitised());

  Stride::Permutation invalid_min({1, 2, 3, 4});
  EXPECT_FALSE(invalid_min.is_sanitised());

  Stride::Permutation invalid_max({0, 1, 2, 5});
  EXPECT_FALSE(invalid_max.is_sanitised());

  Stride::Permutation duplicate({0, 1, 1, 3});
  EXPECT_FALSE(duplicate.is_sanitised());
}

TEST_F(PermutationTest, Sanitise) {
  Stride::Permutation perm({3, 1, 2, 0});
  perm.sanitise();
  EXPECT_TRUE(perm.is_sanitised());
}

TEST_F(PermutationTest, Valid) {
  Stride::Permutation valid({0, 1, 2, 3});
  EXPECT_TRUE(valid.valid());

  Stride::Permutation negative({-1, 0, 1, 2});
  EXPECT_FALSE(negative.valid());
}

TEST_F(PermutationTest, Resize) {
  Stride::Permutation perm({0, 1, 2, 3});
  perm.resize(2);
  EXPECT_EQ(perm.size(), 2);
  EXPECT_EQ(perm[0], 0);
  EXPECT_EQ(perm[1], 1);
}

TEST_F(PermutationTest, Head) {
  Stride::Permutation perm({0, 1, 2, 3});
  auto head = perm.head(2);
  EXPECT_EQ(head.size(), 2);
  EXPECT_EQ(head[0], 0);
  EXPECT_EQ(head[1], 1);
}

TEST_F(PermutationTest, AxisRange) {
  auto perm = Stride::Permutation::axis_range(2, 5);
  EXPECT_EQ(perm.size(), 3);
  EXPECT_EQ(perm[0], 2);
  EXPECT_EQ(perm[1], 3);
  EXPECT_EQ(perm[2], 4);
  EXPECT_TRUE(perm.is_sanitised());
}

TEST_F(PermutationTest, Canonical) {
  auto perm = Stride::Permutation::canonical(5);
  EXPECT_EQ(perm.size(), 5);
  EXPECT_TRUE(perm.is_canonical());
  EXPECT_TRUE(perm.is_sanitised());
  for (size_t i = 0; i < 5; ++i)
    EXPECT_EQ(perm[i], i);
}

TEST_F(PermutationTest, ContiguousAlongAxis) {
  auto perm = Stride::Permutation::contiguous_along_axis(4, 2);
  EXPECT_EQ(perm.size(), 4);
  EXPECT_EQ(perm[2], 0);
  EXPECT_EQ(perm[0], 1);
  EXPECT_EQ(perm[1], 1);
  EXPECT_EQ(perm[3], 1);
}

TEST_F(PermutationTest, ContiguousAlongSpatialAxes) {
  auto perm = Stride::Permutation::contiguous_along_spatial_axes(5);
  EXPECT_EQ(perm.size(), 5);
  EXPECT_EQ(perm[0], 0);
  EXPECT_EQ(perm[1], 0);
  EXPECT_EQ(perm[2], 0);
  EXPECT_EQ(perm[3], 1);
  EXPECT_EQ(perm[4], 1);
}

// Test with mismatched dimensions
TEST_F(PermutationTest, MismatchedDimensionsFromSymbolic) {
  Stride::Symbolic symbolic_3d({1, 2, 3});
  Stride::Permutation perm_3d(symbolic_3d);
  EXPECT_EQ(perm_3d.size(), 3);

  Stride::Symbolic symbolic_5d({1, 2, 3, 4, 5});
  Stride::Permutation perm_5d(symbolic_5d);
  EXPECT_EQ(perm_5d.size(), 5);
}

// =============================================================================
// Symbolic class tests (New implementation)
// =============================================================================

class SymbolicTest : public ::testing::Test {};

TEST_F(SymbolicTest, ConstructFromVector) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  EXPECT_EQ(symbolic.size(), 4);
  EXPECT_TRUE(symbolic.is_canonical());
}

TEST_F(SymbolicTest, ConstructFromActual) {
  Stride::Actual actual({1, 10, 100, 1000});
  Stride::Symbolic symbolic(actual);
  EXPECT_EQ(symbolic.size(), 4);
  EXPECT_EQ(symbolic[0], 1);
  EXPECT_EQ(symbolic[1], 2);
  EXPECT_EQ(symbolic[2], 3);
  EXPECT_EQ(symbolic[3], 4);
}

TEST_F(SymbolicTest, IsCanonical) {
  Stride::Symbolic canonical({1, 2, 3, 4});
  EXPECT_TRUE(canonical.is_canonical());

  Stride::Symbolic non_canonical({3, 1, 2, 4});
  EXPECT_FALSE(non_canonical.is_canonical());
}

TEST_F(SymbolicTest, IsDegenerate) {
  Stride::Symbolic valid({1, 2, 3, 4});
  EXPECT_FALSE(valid.is_degenerate());

  Stride::Symbolic::vector_type degenerate_vec{1, 2, 2, 4};
  auto degenerate = Stride::Symbolic(degenerate_vec);
  // After sanitization, duplicates should be removed
  EXPECT_FALSE(degenerate.is_degenerate());
}

TEST_F(SymbolicTest, IsSanitised) {
  Stride::Symbolic sanitised({1, 2, 3, 4});
  EXPECT_TRUE(sanitised.is_sanitised());
}

TEST_F(SymbolicTest, Valid) {
  Stride::Symbolic valid({1, 2, 3, 4});
  EXPECT_TRUE(valid.valid());

  // Invalid strides with zeros
  Stride::Symbolic::vector_type invalid_vec{1, 0, 3, 4};
  // Constructor sanitises, so need to test with presanitised data
  Stride::Symbolic invalid(invalid_vec);
  EXPECT_TRUE(invalid.valid()); // After sanitisation, zeros are replaced
}

TEST_F(SymbolicTest, Flip) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  symbolic.flip(1);
  EXPECT_EQ(symbolic[0], 1);
  EXPECT_EQ(symbolic[1], -2);
  EXPECT_EQ(symbolic[2], 3);
  EXPECT_EQ(symbolic[3], 4);
}

TEST_F(SymbolicTest, Head) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  auto head = symbolic.head(2);
  EXPECT_EQ(head.size(), 2);
  EXPECT_EQ(head[0], 1);
  EXPECT_EQ(head[1], 2);
}

TEST_F(SymbolicTest, Block) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  auto block = symbolic.block(1, 3);
  EXPECT_EQ(block.size(), 2);
  EXPECT_EQ(block[0], 1);
  EXPECT_EQ(block[1], 2);
}

TEST_F(SymbolicTest, Resize) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  symbolic.resize(6);
  EXPECT_EQ(symbolic.size(), 6);
  EXPECT_EQ(symbolic[0], 1);
  EXPECT_EQ(symbolic[1], 2);
  EXPECT_EQ(symbolic[2], 3);
  EXPECT_EQ(symbolic[3], 4);
  EXPECT_EQ(symbolic[4], 0);
  EXPECT_EQ(symbolic[5], 0);
}

TEST_F(SymbolicTest, Resized) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  auto resized = symbolic.resized(2);
  EXPECT_EQ(resized.size(), 2);
  EXPECT_EQ(symbolic.size(), 4); // Original unchanged
}

TEST_F(SymbolicTest, Conform) {
  Stride::Symbolic symbolic({2, 1, 3, 4});
  Stride::Symbolic target({1, 2, 3, 4});
  symbolic.conform(target);
  EXPECT_TRUE(symbolic.is_sanitised());
}

TEST_F(SymbolicTest, Conformed) {
  Stride::Symbolic symbolic({2, 1, 3, 4});
  Stride::Symbolic target({1, 2, 3, 4});
  auto conformed = symbolic.conformed(target);
  EXPECT_TRUE(conformed.is_sanitised());
  // Original unchanged
  EXPECT_EQ(symbolic[0], 1);
  EXPECT_EQ(symbolic[1], 2);
}

TEST_F(SymbolicTest, Reorder) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  Stride::Permutation perm({1, 0, 2, 3});
  symbolic.reorder(perm);
  EXPECT_TRUE(symbolic.valid());
}

TEST_F(SymbolicTest, Reordered) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  Stride::Permutation perm({1, 0, 2, 3});
  auto reordered = symbolic.reordered(perm);
  EXPECT_TRUE(reordered.valid());
  // Original unchanged
  EXPECT_EQ(symbolic[0], 1);
  EXPECT_EQ(symbolic[1], 2);
}

TEST_F(SymbolicTest, Canonical) {
  auto symbolic = Stride::Symbolic::canonical(5);
  EXPECT_EQ(symbolic.size(), 5);
  EXPECT_TRUE(symbolic.is_canonical());
  for (size_t i = 0; i < 5; ++i)
    EXPECT_EQ(symbolic[i], i + 1);
}

TEST_F(SymbolicTest, NegativeStrides) {
  Stride::Symbolic symbolic({-1, -2, -3, -4});
  EXPECT_TRUE(symbolic.is_sanitised());
  EXPECT_EQ(symbolic[0], -1);
  EXPECT_EQ(symbolic[1], -2);
  EXPECT_EQ(symbolic[2], -3);
  EXPECT_EQ(symbolic[3], -4);
}

// Test with mismatched dimensions
TEST_F(SymbolicTest, MismatchedDimensions) {
  Stride::Symbolic symbolic_3d({1, 2, 3});
  EXPECT_EQ(symbolic_3d.size(), 3);

  Stride::Symbolic symbolic_5d({1, 2, 3, 4, 5});
  EXPECT_EQ(symbolic_5d.size(), 5);
}

// =============================================================================
// Actual class tests (New implementation)
// =============================================================================

class ActualTest : public ::testing::Test {};

TEST_F(ActualTest, ConstructFromVector) {
  Stride::Actual actual({1, 10, 100, 1000});
  EXPECT_EQ(actual.size(), 4);
  EXPECT_EQ(actual[0], 1);
  EXPECT_EQ(actual[1], 10);
  EXPECT_EQ(actual[2], 100);
  EXPECT_EQ(actual[3], 1000);
}

TEST_F(ActualTest, ConstructFromSymbolicAndSizes) {
  Stride::Symbolic symbolic({1, 2, 3, 4});
  std::vector<size_t> sizes{10, 20, 30, 40};
  Stride::Actual actual(symbolic, sizes);
  EXPECT_EQ(actual.size(), 4);
  EXPECT_EQ(actual[0], 1);
  EXPECT_EQ(actual[1], 10);
  EXPECT_EQ(actual[2], 200);
  EXPECT_EQ(actual[3], 6000);
}

TEST_F(ActualTest, ConstructFromMockHeader) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  Stride::Actual actual(header);
  EXPECT_EQ(actual.size(), 3);
}

TEST_F(ActualTest, IsCanonical) {
  Stride::Actual canonical({1, 10, 100, 1000});
  EXPECT_TRUE(canonical.is_canonical());

  Stride::Actual non_canonical({100, 1, 10, 1000});
  EXPECT_FALSE(non_canonical.is_canonical());
}

TEST_F(ActualTest, Valid) {
  Stride::Actual valid({1, 10, 100, 1000});
  EXPECT_TRUE(valid.valid());

  Stride::Actual with_zero({0, 10, 100, 1000});
  EXPECT_FALSE(with_zero.valid());

  Stride::Actual duplicate({1, 10, 10, 1000});
  EXPECT_FALSE(duplicate.valid());
}

TEST_F(ActualTest, ToSymbolic) {
  Stride::Actual actual({1, 10, 100, 1000});
  Stride::Symbolic symbolic = actual.symbolic();
  EXPECT_EQ(symbolic.size(), 4);
  EXPECT_EQ(symbolic[0], 1);
  EXPECT_EQ(symbolic[1], 2);
  EXPECT_EQ(symbolic[2], 3);
  EXPECT_EQ(symbolic[3], 4);
}

TEST_F(ActualTest, NegativeStrides) {
  Stride::Actual actual({-1, -10, -100, -1000});
  EXPECT_TRUE(actual.valid());

  Stride::Symbolic symbolic = actual.symbolic();
  EXPECT_EQ(symbolic[0], -1);
  EXPECT_EQ(symbolic[1], -2);
  EXPECT_EQ(symbolic[2], -3);
  EXPECT_EQ(symbolic[3], -4);
}

TEST_F(ActualTest, Match) {
  MockHeader header1({10, 20, 30}, {1, 10, 200});
  MockHeader header2({10, 20, 30}, {1, 10, 200});
  MockHeader header3({10, 20, 30}, {1, 20, 200});

  Stride::Actual actual(header1);
  EXPECT_TRUE(actual.match(header2));
  EXPECT_FALSE(actual.match(header3));
}

// Test with mismatched dimensions
TEST_F(ActualTest, MismatchedDimensions) {
  Stride::Symbolic symbolic_3d({1, 2, 3});
  std::vector<size_t> sizes_3d{10, 20, 30};
  Stride::Actual actual_3d(symbolic_3d, sizes_3d);
  EXPECT_EQ(actual_3d.size(), 3);

  Stride::Symbolic symbolic_5d({1, 2, 3, 4, 5});
  std::vector<size_t> sizes_5d{10, 20, 30, 40, 50};
  Stride::Actual actual_5d(symbolic_5d, sizes_5d);
  EXPECT_EQ(actual_5d.size(), 5);
}

// =============================================================================
// Offset function tests (New implementation)
// =============================================================================

class OffsetTest : public ::testing::Test {};

TEST_F(OffsetTest, PositiveStrides) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  auto offset = Stride::offset(header);
  EXPECT_EQ(offset, 0);
}

TEST_F(OffsetTest, NegativeStrides) {
  MockHeader header({10, 20, 30}, {-1, -10, -200});
  auto offset = Stride::offset(header);
  EXPECT_EQ(offset, 9 + 190 + 5800);
}

TEST_F(OffsetTest, MixedStrides) {
  MockHeader header({10, 20, 30}, {1, -10, 200});
  auto offset = Stride::offset(header);
  EXPECT_EQ(offset, 190);
}

TEST_F(OffsetTest, WithActualStrides) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  Stride::Actual actual(header);
  auto offset = Stride::offset(actual, header);
  EXPECT_EQ(offset, 0);
}

// =============================================================================
// Legacy namespace tests
// =============================================================================

class LegacyGetSetTest : public ::testing::Test {};

TEST_F(LegacyGetSetTest, GetStrides) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  auto strides = Stride::Legacy::get(header);
  EXPECT_EQ(strides.size(), 3);
  EXPECT_EQ(strides[0], 1);
  EXPECT_EQ(strides[1], 10);
  EXPECT_EQ(strides[2], 200);
}

TEST_F(LegacyGetSetTest, SetStrides) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  Stride::Legacy::List strides{2, 3, 1};
  Stride::Legacy::set(header, strides);
  EXPECT_EQ(header.stride(0), 2);
  EXPECT_EQ(header.stride(1), 3);
  EXPECT_EQ(header.stride(2), 1);
}

TEST_F(LegacyGetSetTest, SetStridesFromAnotherHeader) {
  MockHeader header1({10, 20, 30}, {1, 10, 200});
  MockHeader header2({15, 25, 35}, {2, 3, 1});
  Stride::Legacy::set(header1, header2);
  EXPECT_EQ(header1.stride(0), 2);
  EXPECT_EQ(header1.stride(1), 3);
  EXPECT_EQ(header1.stride(2), 1);
}

// Test with mismatched dimensions
TEST_F(LegacyGetSetTest, SetStridesMismatchedDimensions) {
  MockHeader header({10, 20, 30, 40}, {1, 10, 200, 6000});
  Stride::Legacy::List strides{2, 3};
  Stride::Legacy::set(header, strides);
  EXPECT_EQ(header.stride(0), 2);
  EXPECT_EQ(header.stride(1), 3);
  EXPECT_EQ(header.stride(2), 0);
  EXPECT_EQ(header.stride(3), 0);
}

class LegacyOrderTest : public ::testing::Test {};

TEST_F(LegacyOrderTest, OrderCanonical) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  auto order = Stride::Legacy::order(header);
  EXPECT_EQ(order.size(), 3);
  EXPECT_EQ(order[0], 0);
  EXPECT_EQ(order[1], 1);
  EXPECT_EQ(order[2], 2);
}

TEST_F(LegacyOrderTest, OrderNonCanonical) {
  MockHeader header({10, 20, 30}, {200, 1, 10});
  auto order = Stride::Legacy::order(header);
  EXPECT_EQ(order.size(), 3);
  EXPECT_EQ(order[0], 1);
  EXPECT_EQ(order[1], 2);
  EXPECT_EQ(order[2], 0);
}

TEST_F(LegacyOrderTest, OrderFromList) {
  Stride::Legacy::List strides{3, 1, 2};
  auto order = Stride::Legacy::order(strides);
  EXPECT_EQ(order.size(), 3);
  EXPECT_EQ(order[0], 1);
  EXPECT_EQ(order[1], 2);
  EXPECT_EQ(order[2], 0);
}

TEST_F(LegacyOrderTest, OrderWithRange) {
  MockHeader header({10, 20, 30, 40}, {1, 10, 200, 6000});
  auto order = Stride::Legacy::order(header, 1, 3);
  EXPECT_EQ(order.size(), 2);
  EXPECT_EQ(order[0], 1);
  EXPECT_EQ(order[1], 2);
}

// Test with mismatched dimensions
TEST_F(LegacyOrderTest, OrderMismatchedDimensions) {
  Stride::Legacy::List strides_3d{1, 2, 3};
  auto order_3d = Stride::Legacy::order(strides_3d);
  EXPECT_EQ(order_3d.size(), 3);

  Stride::Legacy::List strides_5d{1, 2, 3, 4, 5};
  auto order_5d = Stride::Legacy::order(strides_5d);
  EXPECT_EQ(order_5d.size(), 5);
}

class LegacySanitiseTest : public ::testing::Test {};

TEST_F(LegacySanitiseTest, SanitiseDuplicates) {
  MockHeader header({10, 20, 30}, {1, 1, 2});
  Stride::Legacy::sanitise(header);
  EXPECT_NE(header.stride(0), header.stride(1));
  EXPECT_NE(header.stride(0), header.stride(2));
  EXPECT_NE(header.stride(1), header.stride(2));
}

TEST_F(LegacySanitiseTest, SanitiseZeros) {
  MockHeader header({10, 20, 30}, {1, 0, 2});
  Stride::Legacy::sanitise(header);
  EXPECT_NE(header.stride(0), 0);
  EXPECT_NE(header.stride(1), 0);
  EXPECT_NE(header.stride(2), 0);
}

TEST_F(LegacySanitiseTest, SanitiseWithList) {
  Stride::Legacy::List strides{1, 0, 2};
  MockHeader header({10, 20, 30}, {1, 10, 200});
  Stride::Legacy::sanitise(strides, header);
  EXPECT_NE(strides[0], 0);
  EXPECT_NE(strides[1], 0);
  EXPECT_NE(strides[2], 0);
}

TEST_F(LegacySanitiseTest, SanitiseUnitySizeAxis) {
  MockHeader header({10, 1, 30}, {1, 2, 3});
  Stride::Legacy::sanitise(header);
  // Axis with size 1 should have stride set to 0 initially, then sanitised
  EXPECT_NE(header.stride(0), 0);
  EXPECT_NE(header.stride(2), 0);
}

// Test with mismatched dimensions
TEST_F(LegacySanitiseTest, SanitiseMismatchedDimensions) {
  MockHeader header_3d({10, 20, 30}, {1, 0, 2});
  Stride::Legacy::sanitise(header_3d);
  EXPECT_NE(header_3d.stride(1), 0);

  MockHeader header_5d({10, 20, 30, 40, 50}, {1, 0, 2, 0, 3});
  Stride::Legacy::sanitise(header_5d);
  EXPECT_NE(header_5d.stride(1), 0);
  EXPECT_NE(header_5d.stride(3), 0);
}

class LegacyActualiseTest : public ::testing::Test {};

TEST_F(LegacyActualiseTest, ActualiseSymbolic) {
  MockHeader header({10, 20, 30}, {1, 2, 3});
  Stride::Legacy::actualise(header);
  EXPECT_EQ(header.stride(0), 1);
  EXPECT_EQ(header.stride(1), 10);
  EXPECT_EQ(header.stride(2), 200);
}

TEST_F(LegacyActualiseTest, ActualiseNegative) {
  MockHeader header({10, 20, 30}, {-1, -2, -3});
  Stride::Legacy::actualise(header);
  EXPECT_EQ(header.stride(0), -1);
  EXPECT_EQ(header.stride(1), -10);
  EXPECT_EQ(header.stride(2), -200);
}

TEST_F(LegacyActualiseTest, ActualiseWithList) {
  Stride::Legacy::List strides{3, 1, 2};
  MockHeader header({10, 20, 30}, {1, 10, 200});
  Stride::Legacy::actualise(strides, header);
  EXPECT_EQ(strides[0], 200);
  EXPECT_EQ(strides[1], 1);
  EXPECT_EQ(strides[2], 10);
}

TEST_F(LegacyActualiseTest, GetActual) {
  MockHeader header({10, 20, 30}, {1, 2, 3});
  auto actual = Stride::Legacy::get_actual(header);
  EXPECT_EQ(actual[0], 1);
  EXPECT_EQ(actual[1], 10);
  EXPECT_EQ(actual[2], 200);
}

// Test with mismatched dimensions
TEST_F(LegacyActualiseTest, ActualiseMismatchedDimensions) {
  MockHeader header_3d({10, 20, 30}, {1, 2, 3});
  Stride::Legacy::actualise(header_3d);
  EXPECT_EQ(header_3d.stride(0), 1);
  EXPECT_EQ(header_3d.stride(1), 10);
  EXPECT_EQ(header_3d.stride(2), 200);

  MockHeader header_5d({10, 20, 30, 40, 50}, {1, 2, 3, 4, 5});
  Stride::Legacy::actualise(header_5d);
  EXPECT_EQ(header_5d.stride(0), 1);
  EXPECT_EQ(header_5d.stride(1), 10);
  EXPECT_EQ(header_5d.stride(2), 200);
  EXPECT_EQ(header_5d.stride(3), 6000);
  EXPECT_EQ(header_5d.stride(4), 240000);
}

class LegacySymboliseTest : public ::testing::Test {};

TEST_F(LegacySymboliseTest, SymboliseActual) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  Stride::Legacy::symbolise(header);
  EXPECT_EQ(header.stride(0), 1);
  EXPECT_EQ(header.stride(1), 2);
  EXPECT_EQ(header.stride(2), 3);
}

TEST_F(LegacySymboliseTest, SymboliseNegative) {
  MockHeader header({10, 20, 30}, {-1, -10, -200});
  Stride::Legacy::symbolise(header);
  EXPECT_EQ(header.stride(0), -1);
  EXPECT_EQ(header.stride(1), -2);
  EXPECT_EQ(header.stride(2), -3);
}

TEST_F(LegacySymboliseTest, SymboliseList) {
  Stride::Legacy::List strides{1, 10, 200};
  Stride::Legacy::symbolise(strides);
  EXPECT_EQ(strides[0], 1);
  EXPECT_EQ(strides[1], 2);
  EXPECT_EQ(strides[2], 3);
}

TEST_F(LegacySymboliseTest, GetSymbolic) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  auto symbolic = Stride::Legacy::get_symbolic(header);
  EXPECT_EQ(symbolic[0], 1);
  EXPECT_EQ(symbolic[1], 2);
  EXPECT_EQ(symbolic[2], 3);
}

// Test with mismatched dimensions
TEST_F(LegacySymboliseTest, SymboliseMismatchedDimensions) {
  Stride::Legacy::List strides_3d{1, 10, 200};
  Stride::Legacy::symbolise(strides_3d);
  EXPECT_EQ(strides_3d.size(), 3);

  Stride::Legacy::List strides_5d{1, 10, 200, 6000, 240000};
  Stride::Legacy::symbolise(strides_5d);
  EXPECT_EQ(strides_5d.size(), 5);
}

class LegacyOffsetTest : public ::testing::Test {};

TEST_F(LegacyOffsetTest, OffsetPositiveStrides) {
  MockHeader header({10, 20, 30}, {1, 10, 200});
  auto offset = Stride::Legacy::offset(header);
  EXPECT_EQ(offset, 0);
}

TEST_F(LegacyOffsetTest, OffsetNegativeStrides) {
  MockHeader header({10, 20, 30}, {-1, -10, -200});
  auto offset = Stride::Legacy::offset(header);
  EXPECT_EQ(offset, 9 + 190 + 5800);
}

TEST_F(LegacyOffsetTest, OffsetMixedStrides) {
  MockHeader header({10, 20, 30}, {1, -10, 200});
  auto offset = Stride::Legacy::offset(header);
  EXPECT_EQ(offset, 190);
}

TEST_F(LegacyOffsetTest, OffsetWithList) {
  Stride::Legacy::List strides{-1, -10, -200};
  MockHeader header({10, 20, 30}, {1, 10, 200});
  auto offset = Stride::Legacy::offset(strides, header);
  EXPECT_EQ(offset, 9 + 190 + 5800);
}

class LegacyGetNearestMatchTest : public ::testing::Test {};

TEST_F(LegacyGetNearestMatchTest, ExactMatch) {
  MockHeader header({10, 20, 30, 40}, {1, 2, 3, 4});
  Stride::Legacy::List desired{0, 0, 0, 1};
  auto result = Stride::Legacy::get_nearest_match(header, desired);
  EXPECT_EQ(result.size(), 4);
}

TEST_F(LegacyGetNearestMatchTest, NoMatch) {
  MockHeader header({10, 20, 30, 40}, {3, 2, 4, 1});
  Stride::Legacy::List desired{0, 0, 0, 1};
  auto result = Stride::Legacy::get_nearest_match(header, desired);
  EXPECT_EQ(result.size(), 4);
}

TEST_F(LegacyGetNearestMatchTest, PartialMatch) {
  MockHeader header({10, 20, 30, 40}, {1, 2, 3, 4});
  Stride::Legacy::List desired{1, 2, 0, 0};
  auto result = Stride::Legacy::get_nearest_match(header, desired);
  EXPECT_EQ(result.size(), 4);
}

// Test with mismatched dimensions
TEST_F(LegacyGetNearestMatchTest, MismatchedDimensions) {
  MockHeader header({10, 20, 30, 40}, {1, 2, 3, 4});
  Stride::Legacy::List desired{1, 2};
  auto result = Stride::Legacy::get_nearest_match(header, desired);
  EXPECT_EQ(result.size(), 4);
}

class LegacyContiguousTest : public ::testing::Test {};

TEST_F(LegacyContiguousTest, ContiguousAlongAxis) {
  auto strides = Stride::Legacy::contiguous_along_axis(3);
  EXPECT_EQ(strides.size(), 4);
  EXPECT_EQ(strides[3], 1);
  EXPECT_EQ(strides[0], 0);
  EXPECT_EQ(strides[1], 0);
  EXPECT_EQ(strides[2], 0);
}

TEST_F(LegacyContiguousTest, ContiguousAlongAxisWithHeader) {
  MockHeader header({10, 20, 30, 40}, {1, 2, 3, 4});
  auto strides = Stride::Legacy::contiguous_along_axis(3, header);
  EXPECT_EQ(strides.size(), 4);
}

TEST_F(LegacyContiguousTest, ContiguousAlongSpatialAxes) {
  MockHeader header({10, 20, 30, 40, 50}, {1, 2, 3, 4, 5});
  auto strides = Stride::Legacy::contiguous_along_spatial_axes(header);
  EXPECT_EQ(strides.size(), 5);
  // First 3 axes (spatial) should be preserved
  EXPECT_EQ(strides[0], 1);
  EXPECT_EQ(strides[1], 2);
  EXPECT_EQ(strides[2], 3);
  // Axes 3+ should be set to 0
  EXPECT_EQ(strides[3], 0);
  EXPECT_EQ(strides[4], 0);
}

// Test with mismatched dimensions
TEST_F(LegacyContiguousTest, ContiguousMismatchedDimensions) {
  auto strides_2d = Stride::Legacy::contiguous_along_axis(1);
  EXPECT_EQ(strides_2d.size(), 2);

  auto strides_5d = Stride::Legacy::contiguous_along_axis(4);
  EXPECT_EQ(strides_5d.size(), 5);
}

// =============================================================================
// Integration tests: New vs Legacy
// =============================================================================

class IntegrationTest : public ::testing::Test {};

TEST_F(IntegrationTest, SymbolicToActualConsistency) {
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

TEST_F(IntegrationTest, ActualToSymbolicConsistency) {
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

TEST_F(IntegrationTest, OrderConsistency) {
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

TEST_F(IntegrationTest, OffsetConsistency) {
  // New implementation
  MockHeader header1({10, 20, 30}, {-1, -10, -200});
  auto offset_new = Stride::offset(header1);

  // Legacy implementation
  MockHeader header2({10, 20, 30}, {-1, -10, -200});
  auto offset_legacy = Stride::Legacy::offset(header2);

  // Results should match
  EXPECT_EQ(offset_new, offset_legacy);
}

// =============================================================================
// Invalid value tests
// =============================================================================

class InvalidValueTest : public ::testing::Test {};

TEST_F(InvalidValueTest, OrderInvalidValues) {
  // Empty order
  Stride::Order empty;
  EXPECT_FALSE(empty.valid());

  // Negative values
  Stride::Order negative(std::vector<ArrayIndex>{-1, 0, 1});
  EXPECT_FALSE(negative.valid());
}

TEST_F(InvalidValueTest, PermutationInvalidValues) {
  // Empty permutation
  Stride::Permutation empty;
  EXPECT_FALSE(empty.valid());

  // Negative values
  Stride::Permutation negative({-1, 0, 1});
  EXPECT_FALSE(negative.valid());
}

TEST_F(InvalidValueTest, SymbolicInvalidValues) {
  // Empty symbolic
  Stride::Symbolic empty;
  EXPECT_FALSE(empty.valid());
}

TEST_F(InvalidValueTest, ActualInvalidValues) {
  // Zero strides
  Stride::Actual with_zero({0, 10, 100});
  EXPECT_FALSE(with_zero.valid());

  // Duplicate strides
  Stride::Actual duplicate({10, 10, 100});
  EXPECT_FALSE(duplicate.valid());
}

TEST_F(InvalidValueTest, LegacySanitiseInvalid) {
  // All zeros
  MockHeader header({10, 20, 30}, {0, 0, 0});
  Stride::Legacy::sanitise(header);
  EXPECT_NE(header.stride(0), 0);
  EXPECT_NE(header.stride(1), 0);
  EXPECT_NE(header.stride(2), 0);
}
