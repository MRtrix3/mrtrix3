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

#include "gtest/gtest.h"

#include "math/rng.h"
#include "misc/bitset.h"

namespace {

// Check if two BitSets are identical up to a certain number of bits
bool identical(const MR::BitSet &a, const MR::BitSet &b, const size_t bits) {
  if (a.size() < bits || b.size() < bits)
    return false;
  for (size_t i = 0; i < bits; ++i) {
    if (a[i] != b[i])
      return false;
  }
  return true;
}

// Check if the latter part of a BitSet (from a given bit) is filled with a specific value
bool valid_last(const MR::BitSet &bitset, const size_t from_bit, const bool value) {
  // If from_bit is beyond the actual size, there are no trailing bits to check in that range
  if (from_bit > bitset.size()) {
    return true;
  }
  for (size_t i = from_bit; i < bitset.size(); ++i) {
    if (bitset[i] != value)
      return false;
  }
  return true;
}
} // end anonymous namespace

class BitSetParamTest : public ::testing::TestWithParam<std::tuple<size_t, bool>> {
protected:
  size_t num_bits;
  bool initial_fill_value;
  MR::Math::RNG::Integer<size_t> rng;

public:
  BitSetParamTest()
      : num_bits(std::get<0>(GetParam())),
        initial_fill_value(std::get<1>(GetParam())),
        rng(num_bits > 0 ? (num_bits - 1) : 0) {}
};

TEST_P(BitSetParamTest, ConstructorAndInitialState) {
  const MR::BitSet data(num_bits, initial_fill_value);

  EXPECT_EQ(data.size(), num_bits);

  if (num_bits == 0) {
    EXPECT_TRUE(data.empty()) << "Size 0 BitSet (filled " << initial_fill_value
                              << ") should be empty. Data: " << MR::str(data);
    EXPECT_TRUE(data.full()) << "Size 0 BitSet (filled " << initial_fill_value
                             << ") should be full. Data: " << MR::str(data);
    EXPECT_EQ(data.count(), 0) << "Size 0 BitSet should have count 0. Data: " << MR::str(data);
  } else {
    if (initial_fill_value) { // Filled with true
      EXPECT_FALSE(data.empty()) << "BitSet (size " << num_bits
                                 << ", filled true) reported empty. Data: " << MR::str(data);
      EXPECT_TRUE(data.full()) << "BitSet (size " << num_bits
                               << ", filled true) reported not full. Data: " << MR::str(data);
      EXPECT_EQ(data.count(), num_bits) << "BitSet (size " << num_bits
                                        << ", filled true) count mismatch. Data: " << MR::str(data);
    } else {
      EXPECT_TRUE(data.empty()) << "BitSet (size " << num_bits
                                << ", filled false) reported not empty. Data: " << MR::str(data);
      EXPECT_FALSE(data.full()) << "Non-full BitSet (size " << num_bits
                                << ", filled false) reported full. Data: " << MR::str(data);
      EXPECT_EQ(data.count(), 0) << "BitSet (size " << num_bits
                                 << ", filled false) count mismatch. Data: " << MR::str(data);
    }
  }
}

TEST_P(BitSetParamTest, CopyConstructorAndEquality) {
  const MR::BitSet original(num_bits, initial_fill_value);
  const MR::BitSet copy(original);

  EXPECT_EQ(original.size(), copy.size());
  EXPECT_EQ(original.count(), copy.count());
  EXPECT_EQ(original, copy) << "Original: " << MR::str(original) << ", Copy: " << MR::str(copy);

  if (num_bits > 0) {
    MR::BitSet mutable_copy(original);
    const size_t bit_to_flip = rng();

    mutable_copy[bit_to_flip] = !mutable_copy[bit_to_flip];
    EXPECT_NE(original, mutable_copy) << "Original: " << MR::str(original)
                                      << ", Modified Copy: " << MR::str(mutable_copy);

    mutable_copy[bit_to_flip] = !mutable_copy[bit_to_flip]; // Flip back
    EXPECT_EQ(original, mutable_copy) << "Original: " << MR::str(original)
                                      << ", Restored Copy: " << MR::str(mutable_copy);
  }
}

TEST_P(BitSetParamTest, AssignmentOperator) {
  const MR::BitSet source(num_bits, initial_fill_value);
  const bool other_fill_value = !initial_fill_value;
  // Ensure 'other_num_bits' is different but valid, including 0.
  const size_t other_num_bits = (num_bits < 8 && num_bits != 0) ? (num_bits + 8) : (num_bits / 2);
  MR::BitSet destination(other_num_bits, other_fill_value);

  destination = source;

  EXPECT_EQ(source.size(), destination.size());
  EXPECT_EQ(source.count(), destination.count());
  EXPECT_EQ(source, destination) << "Source: " << MR::str(source)
                                 << ", Destination after assign: " << MR::str(destination);
  EXPECT_TRUE(identical(source, destination, num_bits));
}

TEST_P(BitSetParamTest, BitAccessAndProgressiveModification) {
  MR::BitSet data(num_bits, initial_fill_value);
  const bool value_to_set = !initial_fill_value;

  if (num_bits == 0) {
    EXPECT_TRUE(data.empty());
    EXPECT_TRUE(data.full());
    EXPECT_EQ(data.count(), 0);
    return;
  }

  // Loop `num_bits` times, each time flipping bit `idx` from `initial_fill_value` to `value_to_set`.
  for (size_t flip_count = 0; flip_count < num_bits; ++flip_count) {
    if (initial_fill_value == false) {
      EXPECT_FALSE(data.full()) << "BitSet (size " << num_bits << ") being filled with true should not yet be full. "
                                << "Bits flipped in prior iterations: " << flip_count
                                << ". Current count (true bits): " << data.count() << ". Data: " << MR::str(data);
    } else {
      EXPECT_FALSE(data.empty()) << "BitSet (size " << num_bits << ") being filled with false should not yet be empty. "
                                 << "Bits flipped in prior iterations: " << flip_count
                                 << ". Current count (true bits): " << data.count() << ". Data: " << MR::str(data);
    }

    const size_t index_to_toggle = flip_count;

    ASSERT_EQ(data[index_to_toggle], initial_fill_value)
        << "Bit " << index_to_toggle << " has unexpected value before flipping. " << "Expected: " << initial_fill_value
        << ", Got: " << data[index_to_toggle] << ". Initial fill for BitSet: " << initial_fill_value
        << ". Iteration (idx): " << flip_count << ". Data: " << MR::str(data);

    data[index_to_toggle] = value_to_set;

    // After flipping bit `flip_count`, a total of `flip_count + 1` bits have been flipped to `value_to_set`.
    const size_t num_distinct_bits_now_value_to_set = flip_count + 1;
    const size_t expected_true_bits_count =
        initial_fill_value ? (num_bits - num_distinct_bits_now_value_to_set) : num_distinct_bits_now_value_to_set;

    EXPECT_EQ(data.count(), expected_true_bits_count)
        << "Count mismatch after flipping " << num_distinct_bits_now_value_to_set << " distinct bits (0 to "
        << index_to_toggle << "). " << "Initial fill was: " << initial_fill_value
        << ", target bit value is: " << value_to_set << ". Data: " << MR::str(data)
        << ". Expected true_bits_count: " << expected_true_bits_count << ". Flipped index: " << index_to_toggle;
  }

  if (value_to_set) {
    EXPECT_TRUE(data.full()) << "BitSet should be full after all bits set to true. Data: " << MR::str(data);
    EXPECT_EQ(data.count(), num_bits);
    EXPECT_EQ(data.empty(), num_bits == 0) << "BitSet (all true) empty status incorrect. Data: " << MR::str(data);
  } else {
    EXPECT_TRUE(data.empty()) << "BitSet should be empty after all bits set to false. Data: " << MR::str(data);
    EXPECT_EQ(data.count(), 0);
    EXPECT_EQ(data.full(), num_bits == 0) << "BitSet (all false) full status incorrect. Data: " << MR::str(data);
  }
}

TEST_P(BitSetParamTest, ResizeOperations) {
  const MR::BitSet original_data(num_bits, initial_fill_value);

  // Resize to a larger size, new bits initialized to false
  const size_t larger_size = num_bits + 8;
  MR::BitSet data_resized_larger_false = original_data;
  data_resized_larger_false.resize(larger_size, false);

  EXPECT_EQ(data_resized_larger_false.size(), larger_size);
  EXPECT_TRUE(identical(original_data, data_resized_larger_false, num_bits))
      << "Original data not preserved. Original: " << MR::str(original_data)
      << " Resized (larger, new false): " << MR::str(data_resized_larger_false);
  EXPECT_TRUE(valid_last(data_resized_larger_false, num_bits, false))
      << "New bits not false. Resized (larger, new false): " << MR::str(data_resized_larger_false);
  EXPECT_EQ(data_resized_larger_false.count(), original_data.count()) << "Count incorrect. Resized (larger, new false)";

  // Resize to a larger size, new bits initialized to true
  MR::BitSet data_resized_larger_true = original_data;
  data_resized_larger_true.resize(larger_size, true);

  EXPECT_EQ(data_resized_larger_true.size(), larger_size);
  EXPECT_TRUE(identical(original_data, data_resized_larger_true, num_bits))
      << "Original data not preserved. Original: " << MR::str(original_data)
      << " Resized (larger, new true): " << MR::str(data_resized_larger_true);
  EXPECT_TRUE(valid_last(data_resized_larger_true, num_bits, true))
      << "New bits not true. Resized (larger, new true): " << MR::str(data_resized_larger_true);
  EXPECT_EQ(data_resized_larger_true.count(), original_data.count() + (larger_size - num_bits))
      << "Count incorrect. Resized (larger, new true)";

  // Resize to a smaller size
  if (num_bits > 0) {
    const size_t smaller_size = num_bits / 2;
    MR::BitSet data_resized_smaller = original_data;
    data_resized_smaller.resize(smaller_size, false); // Bool for new bits irrelevant for shrinking

    EXPECT_EQ(data_resized_smaller.size(), smaller_size);
    EXPECT_TRUE(identical(original_data, data_resized_smaller, smaller_size))
        << "Original data not preserved. Original: " << MR::str(original_data)
        << " Resized (smaller): " << MR::str(data_resized_smaller);

    size_t expected_count_smaller = 0;
    for (size_t k = 0; k < smaller_size; ++k) {
      if (original_data[k])
        expected_count_smaller++;
    }
    EXPECT_EQ(data_resized_smaller.count(), expected_count_smaller) << "Count incorrect. Resized (smaller)";
  }

  // Resize to size 0
  MR::BitSet data_resized_zero = original_data;
  data_resized_zero.resize(0, false);
  EXPECT_EQ(data_resized_zero.size(), 0);
  EXPECT_EQ(data_resized_zero.count(), 0);
  EXPECT_TRUE(data_resized_zero.empty());
  EXPECT_TRUE(data_resized_zero.full()); // A BitSet of size 0 is both empty and full.

  // Resize from size 0 to a non-zero size
  MR::BitSet data_from_zero(0, false);
  const size_t target_size_from_zero = 8;
  data_from_zero.resize(target_size_from_zero, true);
  EXPECT_EQ(data_from_zero.size(), target_size_from_zero);
  EXPECT_EQ(data_from_zero.count(), target_size_from_zero);
  EXPECT_FALSE(data_from_zero.empty());
  EXPECT_TRUE(data_from_zero.full());
  EXPECT_TRUE(valid_last(data_from_zero, 0, true));
}

// For empty(), full(), count() on various specific states.
TEST_P(BitSetParamTest, StatePropertiesCoverage) {
  if (num_bits == 0) {
    const MR::BitSet data(0, false);
    EXPECT_EQ(data.size(), 0);
    EXPECT_EQ(data.count(), 0);
    EXPECT_TRUE(data.empty());
    EXPECT_TRUE(data.full());
    return;
  }

  // All bits false
  const MR::BitSet all_false(num_bits, false);
  EXPECT_EQ(all_false.size(), num_bits);
  EXPECT_EQ(all_false.count(), 0);
  EXPECT_TRUE(all_false.empty());
  EXPECT_FALSE(all_false.full());

  // All bits true
  const MR::BitSet all_true(num_bits, true);
  EXPECT_EQ(all_true.size(), num_bits);
  EXPECT_EQ(all_true.count(), num_bits);
  EXPECT_FALSE(all_true.empty());
  EXPECT_TRUE(all_true.full());

  // Mixed state (if num_bits > 1, to have at least one true and one false)
  if (num_bits > 1) {
    MR::BitSet mixed_one_true(num_bits, false);
    mixed_one_true[0] = true;
    EXPECT_EQ(mixed_one_true.size(), num_bits);
    EXPECT_EQ(mixed_one_true.count(), 1);
    EXPECT_FALSE(mixed_one_true.empty());
    EXPECT_FALSE(mixed_one_true.full());

    MR::BitSet mixed_one_false(num_bits, true);
    mixed_one_false[0] = false;
    EXPECT_EQ(mixed_one_false.size(), num_bits);
    EXPECT_EQ(mixed_one_false.count(), num_bits - 1);
    EXPECT_FALSE(mixed_one_false.empty());
    EXPECT_FALSE(mixed_one_false.full());
  }
}

std::vector<std::tuple<size_t, bool>> GetBitSetTestParams() {
  std::vector<std::tuple<size_t, bool>> params;
  for (size_t bits = 0; bits < 16; ++bits) {
    params.emplace_back(bits, false);
    params.emplace_back(bits, true);
  }
  return params;
}

INSTANTIATE_TEST_SUITE_P(BitSetParamTests, BitSetParamTest, ::testing::ValuesIn(GetBitSetTestParams()));
