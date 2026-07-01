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

#include "dwi/tractography/formats/write_buffer.h"
#include "exception.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using MR::DWI::Tractography::Formats::WriteBuffer;

namespace {

//! a sink that records every flush callback invocation for inspection
struct FlushRecorder {
  std::vector<std::vector<std::byte>> flushes; //!< the byte payload of each commit
  size_t invocations = 0;

  WriteBuffer::FlushCallback callback() {
    return [this](const std::byte *data, size_t size, const WriteBuffer::Counts &) {
      ++invocations;
      flushes.emplace_back(data, data + size);
    };
  }

  //! all bytes pushed to the sink, concatenated in commit order
  std::vector<std::byte> concatenated() const {
    std::vector<std::byte> out;
    for (const auto &f : flushes)
      out.insert(out.end(), f.begin(), f.end());
    return out;
  }
};

std::byte b(int value) { return static_cast<std::byte>(value); }

// A non-zero element size is mandatory.
TEST(WriteBuffer, RejectsZeroElementSize) { EXPECT_THROW(WriteBuffer(64, 0), MR::Exception); }

// Capacity is rounded down to a whole number of elements (no element split).
TEST(WriteBuffer, CapacityRoundedToElementMultiple) {
  WriteBuffer buffer(10, 4); // 10 bytes / 4 -> 2 elements -> 8 bytes
  EXPECT_EQ(buffer.capacity(), 8u);
}

// A requested capacity below one element still leaves room for a single element.
TEST(WriteBuffer, CapacityNeverBelowOneElement) {
  WriteBuffer buffer(1, 4);
  EXPECT_EQ(buffer.capacity(), 4u);
}

// Buffered data is not flushed until an explicit commit; the callback then fires
//   exactly once with the byte-exact payload.
TEST(WriteBuffer, ExplicitCommitInvokesCallbackOnce) {
  FlushRecorder sink;
  WriteBuffer buffer(64, 1);
  buffer.set_flush_callback(sink.callback());

  const std::byte payload[] = {b(1), b(2), b(3), b(4)};
  buffer.add(payload, sizeof(payload));
  EXPECT_EQ(sink.invocations, 0u); // nothing flushed yet
  EXPECT_EQ(buffer.size(), 4u);

  buffer.commit();
  ASSERT_EQ(sink.invocations, 1u);
  EXPECT_EQ(buffer.size(), 0u);
  ASSERT_EQ(sink.flushes[0].size(), 4u);
  EXPECT_EQ(sink.concatenated(), (std::vector<std::byte>{b(1), b(2), b(3), b(4)}));
}

// Committing an empty buffer is a no-op (no callback invocation).
TEST(WriteBuffer, EmptyCommitIsNoOp) {
  FlushRecorder sink;
  WriteBuffer buffer(64, 1);
  buffer.set_flush_callback(sink.callback());
  buffer.commit();
  EXPECT_EQ(sink.invocations, 0u);
}

// Crossing the capacity boundary forces a commit before the new bytes are stored.
TEST(WriteBuffer, OverflowForcesCommit) {
  FlushRecorder sink;
  WriteBuffer buffer(4, 2); // capacity 4 bytes
  buffer.set_flush_callback(sink.callback());

  const std::byte first[] = {b(10), b(11)};
  buffer.add(first, sizeof(first));
  EXPECT_EQ(sink.invocations, 0u);

  const std::byte second[] = {b(20), b(21)};
  buffer.add(second, sizeof(second)); // now at capacity, still no overflow
  EXPECT_EQ(sink.invocations, 0u);
  EXPECT_EQ(buffer.size(), 4u);

  const std::byte third[] = {b(30), b(31)};
  buffer.add(third, sizeof(third)); // would overflow -> commit the first 4 bytes
  ASSERT_EQ(sink.invocations, 1u);
  EXPECT_EQ(sink.flushes[0], (std::vector<std::byte>{b(10), b(11), b(20), b(21)}));
  EXPECT_EQ(buffer.size(), 2u); // the third element now resident

  buffer.commit();
  ASSERT_EQ(sink.invocations, 2u);
  EXPECT_EQ(sink.flushes[1], (std::vector<std::byte>{b(30), b(31)}));
}

// A single append larger than the capacity grows the buffer to accommodate it.
TEST(WriteBuffer, OversizedAppendGrowsBuffer) {
  FlushRecorder sink;
  WriteBuffer buffer(4, 2);
  buffer.set_flush_callback(sink.callback());

  const std::byte big[] = {b(1), b(2), b(3), b(4), b(5), b(6)};
  buffer.add(big, sizeof(big));
  EXPECT_EQ(sink.invocations, 0u);
  EXPECT_GE(buffer.capacity(), 6u);
  EXPECT_EQ(buffer.size(), 6u);

  buffer.commit();
  ASSERT_EQ(sink.invocations, 1u);
  EXPECT_EQ(sink.flushes[0], (std::vector<std::byte>{b(1), b(2), b(3), b(4), b(5), b(6)}));
}

// Destruction flushes any residual buffered bytes (commit-on-dtor).
TEST(WriteBuffer, DestructorFlushesResidual) {
  FlushRecorder sink;
  {
    WriteBuffer buffer(64, 1);
    buffer.set_flush_callback(sink.callback());
    const std::byte payload[] = {b(7), b(8)};
    buffer.add(payload, sizeof(payload));
    EXPECT_EQ(sink.invocations, 0u);
  } // dtor commits
  ASSERT_EQ(sink.invocations, 1u);
  EXPECT_EQ(sink.flushes[0], (std::vector<std::byte>{b(7), b(8)}));
}

// The concatenation of all committed payloads is byte-exact with the input
//   stream regardless of how commits were triggered.
TEST(WriteBuffer, ByteExactAcrossMultipleCommits) {
  FlushRecorder sink;
  WriteBuffer buffer(4, 1);
  buffer.set_flush_callback(sink.callback());

  std::vector<std::byte> expected;
  for (int i = 0; i != 17; ++i) {
    const std::byte value = b(i);
    buffer.add(&value, 1);
    expected.push_back(value);
  }
  buffer.commit();

  EXPECT_EQ(sink.concatenated(), expected);
}

} // namespace
