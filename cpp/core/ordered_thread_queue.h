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

#pragma once

#include "thread_queue.h"
#include <set>

namespace MR::Thread {

namespace {

template <class Item> class Ordered {
public:
  Ordered() = default;
  Ordered(const Item &item) : item(item) {}

  Item item;
  size_t index;
};

struct CompareItems {
  template <class Item> bool operator()(const Ordered<Item> *a, const Ordered<Item> *b) const {
    return a->index < b->index;
  }
};

template <class JobType> struct job_is_single_threaded : std::true_type {};
template <class JobType> struct job_is_single_threaded<Multi<JobType>> : std::false_type {};

template <class Item> struct _batch_size<Ordered<Batch<Item>>> {
  _batch_size(const Ordered<Batch<Item>> &item) : n(item.item.num) {}
  operator size_t() const { return n; }
  const size_t n;
};

/***********************************************************************
 *        Source/Pipe/Sink for UNBATCHED ordered queue                 *
 ***********************************************************************/

template <class Item> struct Type<Ordered<Item>> {
  using item = Item;
  using queue = Queue<Ordered<Item>>;
  using reader = typename queue::Reader;
  using writer = typename queue::Writer;
  using read_item = typename reader::Item;
  using write_item = typename writer::Item;
};

template <class Item, class Functor> struct SourceWrapper<Ordered<Item>, Functor> {

  using queued_t = Ordered<Item>;
  using queue_t = typename Type<queued_t>::queue;
  using writer_t = typename Type<queued_t>::writer;
  using functor_t = typename _job<Functor>::member_type;

  writer_t writer;
  functor_t func;
  size_t batch_size;

  SourceWrapper(queue_t &queue, Functor &functor, const queued_t &item)
      : writer(queue), func(_job<Functor>::functor(functor)), batch_size(_batch_size<queued_t>(item)) {}

  void execute() {
    size_t count = 0;
    auto out = writer.placeholder();
    do {
      if (!func(out->item))
        break;
      out->index = count++;
    } while (out.write());
  }
};

template <class Item1, class Functor, class Item2> struct PipeWrapper<Ordered<Item1>, Functor, Ordered<Item2>> {

  using queued1_t = Ordered<Item1>;
  using queued2_t = Ordered<Item2>;
  using queue1_t = typename Type<queued1_t>::queue;
  using queue2_t = typename Type<queued2_t>::queue;
  using reader_t = typename Type<queued1_t>::reader;
  using writer_t = typename Type<queued2_t>::writer;
  using functor_t = typename _job<Functor>::member_type;

  reader_t reader;
  writer_t writer;
  functor_t func;
  const size_t batch_size;

  PipeWrapper(queue1_t &queue_in, Functor &functor, queue2_t &queue_out, const queued2_t &item2)
      : reader(queue_in),
        writer(queue_out),
        func(_job<Functor>::functor(functor)),
        batch_size(_batch_size<queued2_t>(item2)) {}

  void execute() {
    auto in = reader.placeholder();
    auto out = writer.placeholder();
    while (in.read()) {
      if (!func(in->item, out->item))
        break;
      out->index = in->index;
      out.write();
    }
  }
};

template <class Item, class Functor> struct SinkWrapper<Ordered<Item>, Functor> {

  using queued_t = Ordered<Item>;
  using queue_t = typename Type<queued_t>::queue;
  using reader_t = typename Type<queued_t>::reader;
  using functor_t = typename _job<Functor>::member_type;

  reader_t reader;
  functor_t func;

  SinkWrapper(queue_t &queue, Functor &functor) : reader(queue), func(_job<Functor>::functor(functor)) {}

  void execute() {
    size_t expected = 0;
    auto in = reader.placeholder();
    std::set<queued_t *, CompareItems> buffer;
    while (in.read()) {
      if (in->index > expected) {
        buffer.emplace(in.stash());
        continue;
      }
      if (!func(in->item))
        return;
      ++expected;
      while (!buffer.empty() && (*buffer.begin())->index <= expected) {
        if (!func((*buffer.begin())->item))
          return;
        in.recycle(*buffer.begin());
        buffer.erase(buffer.begin());
        ++expected;
      }
    }
  }
};

/***********************************************************************
 *        Source/Pipe/Sink for BATCHED ordered queue                 *
 ***********************************************************************/

template <class Item> struct Type<Ordered<Batch<Item>>> {
  using item = Item;
  using queue = Queue<Ordered<std::vector<Item>>>;
  using reader = typename queue::Reader;
  using writer = typename queue::Writer;
  using read_item = typename reader::Item;
  using write_item = typename writer::Item;
};

template <class Item, class Functor> struct SourceWrapper<Ordered<Batch<Item>>, Functor> {

  using queued_t = Ordered<std::vector<Item>>;
  using passed_t = Ordered<Batch<Item>>;
  using queue_t = typename Type<queued_t>::queue;
  using writer_t = typename Type<queued_t>::writer;
  using functor_t = typename _job<Functor>::member_type;

  writer_t writer;
  functor_t func;
  size_t batch_size;

  SourceWrapper(queue_t &queue, Functor &functor, const passed_t &item)
      : writer(queue), func(_job<Functor>::functor(functor)), batch_size(_batch_size<passed_t>(item)) {}

  void execute() {
    size_t count = 0;
    auto out = writer.placeholder();
    bool stop = false;
    do {
      out->item.resize(batch_size);
      for (size_t n = 0; n < batch_size; ++n) {
        if (!func(out->item[n])) {
          out->item.resize(n);
          stop = true;
          break;
        }
      }
      out->index = count++;
    } while (out.write() && !stop);
  }
};

template <class Item1, class Functor, class Item2>
struct PipeWrapper<Ordered<Batch<Item1>>, Functor, Ordered<Batch<Item2>>> {

  using queued1_t = Ordered<std::vector<Item1>>;
  using queued2_t = Ordered<std::vector<Item2>>;
  using passed2_t = Ordered<Batch<Item2>>;
  using queue1_t = typename Type<queued1_t>::queue;
  using queue2_t = typename Type<queued2_t>::queue;
  using reader_t = typename Type<queued1_t>::reader;
  using writer_t = typename Type<queued2_t>::writer;
  using functor_t = typename _job<Functor>::member_type;

  reader_t reader;
  writer_t writer;
  functor_t func;
  const size_t batch_size;

  PipeWrapper(queue1_t &queue_in, Functor &functor, queue2_t &queue_out, const passed2_t &item2)
      : reader(queue_in),
        writer(queue_out),
        func(_job<Functor>::functor(functor)),
        batch_size(_batch_size<passed2_t>(item2)) {}

  void execute() {
    auto in = reader.placeholder();
    auto out = writer.placeholder();
    while (in.read()) {
      out->item.resize(in->item.size());
      size_t k = 0;
      for (size_t n = 0; n < in->item.size(); ++n) {
        if (func(in->item[n], out->item[k]))
          ++k;
      }
      out->item.resize(k);
      out->index = in->index;
      if (!out.write())
        return;
    }
  }
};

template <class Item, class Functor> struct SinkWrapper<Ordered<Batch<Item>>, Functor> {

  using queued_t = Ordered<std::vector<Item>>;
  using queue_t = typename Type<queued_t>::queue;
  using reader_t = typename Type<queued_t>::reader;
  using functor_t = typename _job<Functor>::member_type;

  reader_t reader;
  functor_t func;

  SinkWrapper(queue_t &queue, Functor &functor) : reader(queue), func(_job<Functor>::functor(functor)) {}

  void execute() {
    size_t expected = 0;
    auto in = reader.placeholder();
    std::set<queued_t *, CompareItems> buffer;
    while (in.read()) {
      if (in->index > expected) {
        buffer.emplace(in.stash());
        continue;
      }
      for (size_t n = 0; n < in->item.size(); ++n)
        if (!func(in->item[n]))
          return;
      ++expected;
      while (!buffer.empty() && (*buffer.begin())->index <= expected) {
        for (size_t n = 0; n < (*buffer.begin())->item.size(); ++n)
          if (!func((*buffer.begin())->item[n]))
            return;
        in.recycle(*buffer.begin());
        buffer.erase(buffer.begin());
        ++expected;
      }
    }
  }
};

} // namespace

template <class Source, class Item1, class Pipe, class Item2, class Sink>
inline void run_ordered_queue(Source &&source,
                              const Item1 &item1,
                              Pipe &&pipe,
                              const Item2 &item2,
                              Sink &&sink,
                              size_t capacity = default_queue_capacity) {
  static_assert(job_is_single_threaded<Source>::value && job_is_single_threaded<Sink>::value,
                "run_ordered_queue can only run with single-threaded source & sink");

  if (_batch_size<Item1>(item1) != _batch_size<Item2>(item2))
    throw Exception("Thread::run_ordered_queue must be run with matching batch sizes across all stages");

  run_queue(std::forward<Source>(source),
            Ordered<Item1>(item1),
            std::forward<Pipe>(pipe),
            Ordered<Item2>(item2),
            std::forward<Sink>(sink),
            capacity);
}

template <class Source, class Item1, class Pipe1, class Item2, class Pipe2, class Item3, class Sink>
inline void run_ordered_queue(Source &&source,
                              const Item1 &item1,
                              Pipe1 &&pipe1,
                              const Item2 &item2,
                              Pipe2 &&pipe2,
                              const Item3 &item3,
                              Sink &&sink,
                              size_t capacity = default_queue_capacity) {
  static_assert(job_is_single_threaded<Source>::value && job_is_single_threaded<Sink>::value,
                "run_ordered_queue can only run with single-threaded source & sink");

  if (_batch_size<Item1>(item1) != _batch_size<Item2>(item2) || _batch_size<Item1>(item1) != _batch_size<Item3>(item3))
    throw Exception("Thread::run_ordered_queue must be run with matching batch sizes across all stages");

  run_queue(std::forward<Source>(source),
            Ordered<Item1>(item1),
            std::forward<Pipe1>(pipe1),
            Ordered<Item2>(item2),
            std::forward<Pipe2>(pipe2),
            Ordered<Item3>(item3),
            std::forward<Sink>(sink),
            capacity);
}

} // namespace MR::Thread
