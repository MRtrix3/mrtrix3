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

#include "ordered_thread_queue.h"
#include "thread_queue.h"
#include "timer.h"

#include <atomic>
#include <cstddef>
#include <iostream>

using namespace MR;

namespace {
std::atomic<size_t> g_items_created_count;
}

struct Item {
  Item() { g_items_created_count++; }
  size_t value = 0;
};

constexpr size_t kDefaultSampleSize = 1'000'000;

struct SourceFunctor {
  SourceFunctor() = default;
  SourceFunctor(const SourceFunctor &) = delete;
  SourceFunctor &operator=(const SourceFunctor &) = delete;
  SourceFunctor(SourceFunctor &&) = default;
  SourceFunctor &operator=(SourceFunctor &&) = default;

  ~SourceFunctor() { DEBUG("SourceFunctor: Sent " + MR::str(count) + " items, last value: " + MR::str(value) + "."); }

  // Operator called by the queue to produce items.
  // Returns false when no more items can be produced.
  bool operator()(Item &item) {
    if (++value > kDefaultSampleSize) {
      return false;
    }
    ++count;
    item.value = value;
    return true;
  }

  size_t count = 0;
  size_t value = 0;
};

struct PipeFunctor {
  bool operator()(const Item &in, Item &out) {
    out = in;
    return true;
  }
};

struct SinkFunctor {
  size_t items_received_count = 0;
  size_t out_of_order_items_count = 0;
  size_t last_item_value = 0;

  SinkFunctor() = default;
  SinkFunctor(const SinkFunctor &) = delete;
  SinkFunctor &operator=(const SinkFunctor &) = delete;
  SinkFunctor(SinkFunctor &&) = default;
  SinkFunctor &operator=(SinkFunctor &&) = default;

  ~SinkFunctor() {
    DEBUG("SinkFunctor: Received " + MR::str(items_received_count) + " items, " + MR::str(out_of_order_items_count) +
          " out of order.");
  }

  bool operator()(const Item &item) {
    ++items_received_count;
    // Check for out-of-order items: current item's value should be greater than the last.
    // The first item (value 1) vs last_item_value (initially 0): 1 <= 0 is false, so no out_of_order.
    if (item.value <= last_item_value) {
      ++out_of_order_items_count;
    }
    last_item_value = item.value;
    return true;
  }
};

class OrderedQueueTest : public ::testing::Test {
protected:
  void SetUp() override { g_items_created_count = 0; }

  void TearDown() override { std::cerr << "\n"; }

  enum class OrderEnforcement : uint8_t { Enforce, DoNotEnforce };
  void PerformChecksAndLog(const MR::Timer &timer, const SinkFunctor &sink, OrderEnforcement enforce_order) const {

    ASSERT_EQ(sink.items_received_count, kDefaultSampleSize)
        << ": Sample size mismatch. Expected " << kDefaultSampleSize << ", got " << sink.items_received_count << ".";

    if (sink.out_of_order_items_count > 0) {
      if (enforce_order == OrderEnforcement::Enforce) {
        GTEST_FAIL() << "Order mismatch (enforced). " << sink.out_of_order_items_count << " items out of order.";
      } else {
        DEBUG("Order mismatch (not enforced). " + MR::str(sink.out_of_order_items_count) + " items out of order.");
      }
    }

    DEBUG("Allocated items: " + MR::str(g_items_created_count.load()) + ", Time taken: " + MR::str(timer.elapsed()) +
          " ns");
  }
};

TEST_F(OrderedQueueTest, Regular2Stage) {
  const MR::Timer timer;

  SourceFunctor source;
  SinkFunctor sink;
  Thread::run_queue(source, Item(), sink);

  PerformChecksAndLog(timer, sink, OrderEnforcement::Enforce);
}

TEST_F(OrderedQueueTest, Batched2Stage) {
  const MR::Timer timer;

  SourceFunctor source;
  SinkFunctor sink;
  Thread::run_queue(source, Thread::batch(Item()), sink);

  PerformChecksAndLog(timer, sink, OrderEnforcement::Enforce);
}

TEST_F(OrderedQueueTest, Regular3Stage) {
  const MR::Timer timer;

  SourceFunctor source;
  SinkFunctor sink;
  Thread::run_queue(source, Item(), Thread::multi(PipeFunctor()), Item(), sink);

  PerformChecksAndLog(timer, sink, OrderEnforcement::DoNotEnforce);
}

TEST_F(OrderedQueueTest, BatchedUnbatched3Stage) {
  const MR::Timer timer;

  SourceFunctor source;
  SinkFunctor sink;
  Thread::run_queue(source, Thread::batch(Item()), Thread::multi(PipeFunctor()), Item(), sink);

  PerformChecksAndLog(timer, sink, OrderEnforcement::DoNotEnforce);
}

TEST_F(OrderedQueueTest, UnbatchedBatched3Stage) {
  const MR::Timer timer;

  SourceFunctor source;
  SinkFunctor sink;
  Thread::run_queue(source, Item(), Thread::multi(PipeFunctor()), Thread::batch(Item()), sink);

  PerformChecksAndLog(timer, sink, OrderEnforcement::DoNotEnforce);
}

TEST_F(OrderedQueueTest, BatchedBatchedRegular3Stage) {
  const MR::Timer timer;

  SourceFunctor source;
  SinkFunctor sink;
  Thread::run_queue(source, Thread::batch(Item()), Thread::multi(PipeFunctor()), Thread::batch(Item()), sink);

  PerformChecksAndLog(timer, sink, OrderEnforcement::DoNotEnforce);
}

TEST_F(OrderedQueueTest, Regular4Stage) {
  const MR::Timer timer;

  SourceFunctor source;
  SinkFunctor sink;
  Thread::run_queue(source, Item(), Thread::multi(PipeFunctor()), Item(), Thread::multi(PipeFunctor()), Item(), sink);

  PerformChecksAndLog(timer, sink, OrderEnforcement::DoNotEnforce);
}

TEST_F(OrderedQueueTest, BatchedUnbatchedUnbatched4Stage) {
  const MR::Timer timer;

  SourceFunctor source;
  SinkFunctor sink;
  Thread::run_queue(
      source, Thread::batch(Item()), Thread::multi(PipeFunctor()), Item(), Thread::multi(PipeFunctor()), Item(), sink);

  PerformChecksAndLog(timer, sink, OrderEnforcement::DoNotEnforce);
}

TEST_F(OrderedQueueTest, UnbatchedBatchedUnbatched4Stage) {
  const MR::Timer timer;

  SourceFunctor source;
  SinkFunctor sink;
  Thread::run_queue(
      source, Item(), Thread::multi(PipeFunctor()), Thread::batch(Item()), Thread::multi(PipeFunctor()), Item(), sink);

  PerformChecksAndLog(timer, sink, OrderEnforcement::DoNotEnforce);
}

TEST_F(OrderedQueueTest, UnbatchedUnbatchedBatchedRegular4Stage) {
  const MR::Timer timer;

  SourceFunctor source;
  SinkFunctor sink;
  Thread::run_queue(
      source, Item(), Thread::multi(PipeFunctor()), Item(), Thread::multi(PipeFunctor()), Thread::batch(Item()), sink);

  PerformChecksAndLog(timer, sink, OrderEnforcement::DoNotEnforce);
}

TEST_F(OrderedQueueTest, OrderedUnbatched3Stage) {
  const MR::Timer timer;

  SourceFunctor source;
  SinkFunctor sink;
  Thread::run_ordered_queue(source, Item(), Thread::multi(PipeFunctor()), Item(), sink);

  PerformChecksAndLog(timer, sink, OrderEnforcement::Enforce);
}

TEST_F(OrderedQueueTest, OrderedBatchedBatched3Stage) {
  const MR::Timer timer;

  SourceFunctor source;
  SinkFunctor sink;
  Thread::run_ordered_queue(source, Thread::batch(Item()), Thread::multi(PipeFunctor()), Thread::batch(Item()), sink);

  PerformChecksAndLog(timer, sink, OrderEnforcement::Enforce);
}

TEST_F(OrderedQueueTest, OrderedUnbatched4Stage) {
  const MR::Timer timer;
  SourceFunctor source;
  SinkFunctor sink;
  Thread::run_ordered_queue(
      source, Item(), Thread::multi(PipeFunctor()), Item(), Thread::multi(PipeFunctor()), Item(), sink);
  PerformChecksAndLog(timer, sink, OrderEnforcement::Enforce);
}

TEST_F(OrderedQueueTest, OrderedBatchedBatchedBatched4Stage) {
  const MR::Timer timer;

  SourceFunctor source;
  SinkFunctor sink;
  Thread::run_ordered_queue(source,
                            Thread::batch(Item()),
                            Thread::multi(PipeFunctor()),
                            Thread::batch(Item()),
                            Thread::multi(PipeFunctor()),
                            Thread::batch(Item()),
                            sink);

  PerformChecksAndLog(timer, sink, OrderEnforcement::Enforce);
}
