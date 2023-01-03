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

#ifndef __mrtrix_ordered_thread_queue_h__
#define __mrtrix_ordered_thread_queue_h__

#include <set>
#include "thread_queue.h"

namespace MR {
  namespace Thread {




    namespace {

      template <class Item>
        class __Ordered { MEMALIGN(__Ordered<Item>)
          public:
            __Ordered () = default;
            __Ordered (const Item& item) : item (item) { }

            Item item;
            size_t index;
        };

        struct CompareItems { NOMEMALIGN
          template <class Item>
            bool operator() (const __Ordered<Item>* a, const __Ordered<Item>* b) const { return a->index < b->index; }
        };




      template <class JobType> struct job_is_single_threaded : std::true_type { NOMEMALIGN };
      template <class JobType> struct job_is_single_threaded< __Multi<JobType>> : std::false_type { NOMEMALIGN };


       template <class Item> struct __batch_size <__Ordered<__Batch<Item>>> { NOMEMALIGN
         __batch_size (const __Ordered<__Batch<Item>>& item) : n (item.item.num) { }
         operator size_t () const { return n; }
         const size_t n;
       };




      /***********************************************************************
       *        Source/Pipe/Sink for UNBATCHED ordered queue                 *
       ***********************************************************************/


       template <class Item> struct Type<__Ordered<Item>> { NOMEMALIGN
         using item = Item;
         using queue = Queue<__Ordered<Item>>;
         using reader = typename queue::Reader;
         using writer = typename queue::Writer;
         using read_item = typename reader::Item;
         using write_item = typename writer::Item;
       };


       template <class Item, class Functor>
         struct __Source<__Ordered<Item>,Functor> {
           MEMALIGN(__Source<__Ordered<Item>,Functor>)

           using queued_t = __Ordered<Item>;
           using queue_t = typename Type<queued_t>::queue;
           using writer_t = typename Type<queued_t>::writer;
           using functor_t = typename __job<Functor>::member_type;

           writer_t writer;
           functor_t func;
           size_t batch_size;

           __Source (queue_t& queue, Functor& functor, const queued_t& item) :
             writer (queue),
             func (__job<Functor>::functor (functor)),
             batch_size (__batch_size<queued_t> (item)) { }

           void execute () {
             size_t count = 0;
             auto out = writer.placeholder();
             do {
               if (!func (out->item))
                 break;
               out->index = count++;
             } while (out.write());
           }
         };







       template <class Item1, class Functor, class Item2>
         struct __Pipe<__Ordered<Item1>,Functor,__Ordered<Item2>> {
           MEMALIGN(__Pipe<__Ordered<Item1>,Functor,__Ordered<Item2>>)

           using queued1_t = __Ordered<Item1>;
           using queued2_t = __Ordered<Item2>;
           using queue1_t = typename Type<queued1_t>::queue;
           using queue2_t = typename Type<queued2_t>::queue;
           using reader_t = typename Type<queued1_t>::reader;
           using writer_t = typename Type<queued2_t>::writer;
           using functor_t = typename __job<Functor>::member_type;

           reader_t reader;
           writer_t writer;
           functor_t func;
           const size_t batch_size;

           __Pipe (queue1_t& queue_in, Functor& functor, queue2_t& queue_out, const queued2_t& item2) :
             reader (queue_in),
             writer (queue_out),
             func (__job<Functor>::functor (functor)),
             batch_size (__batch_size<queued2_t> (item2)) { }

           void execute () {
             auto in = reader.placeholder();
             auto out = writer.placeholder();
             while (in.read()) {
               if (!func (in->item, out->item))
                 break;
               out->index = in->index;
               out.write();
             }
           }

         };






       template <class Item, class Functor>
         struct __Sink<__Ordered<Item>,Functor> {
           MEMALIGN(__Sink<__Ordered<Item>,Functor>)

           using queued_t = __Ordered<Item>;
           using queue_t = typename Type<queued_t>::queue;
           using reader_t = typename Type<queued_t>::reader;
           using functor_t = typename __job<Functor>::member_type;

           reader_t reader;
           functor_t func;

           __Sink (queue_t& queue, Functor& functor) :
             reader (queue),
             func (__job<Functor>::functor (functor)) { }

           void execute () {
             size_t expected = 0;
             auto in = reader.placeholder();
             std::set<queued_t*,CompareItems> buffer;
             while (in.read()) {
               if (in->index > expected) {
                 buffer.emplace (in.stash());
                 continue;
               }
               if (!func (in->item))
                 return;
               ++expected;
               while (!buffer.empty() && (*buffer.begin())->index <= expected) {
                 if (!func ((*buffer.begin())->item))
                   return;
                 in.recycle (*buffer.begin());
                 buffer.erase (buffer.begin());
                 ++expected;
               }
             }
           }
         };








      /***********************************************************************
       *        Source/Pipe/Sink for BATCHED ordered queue                 *
       ***********************************************************************/

       template <class Item> struct Type<__Ordered<__Batch<Item>>> { NOMEMALIGN
         using item = Item;
         using queue = Queue<__Ordered<vector<Item>>>;
         using reader = typename queue::Reader;
         using writer = typename queue::Writer;
         using read_item = typename reader::Item;
         using write_item = typename writer::Item;
       };



       template <class Item, class Functor>
         struct __Source<__Ordered<__Batch<Item>>,Functor> {
           MEMALIGN(__Source<__Ordered<__Batch<Item>>,Functor>)

           using queued_t = __Ordered<vector<Item>>;
           using passed_t = __Ordered<__Batch<Item>>;
           using queue_t = typename Type<queued_t>::queue;
           using writer_t = typename Type<queued_t>::writer;
           using functor_t = typename __job<Functor>::member_type;

           writer_t writer;
           functor_t func;
           size_t batch_size;

           __Source (queue_t& queue, Functor& functor, const passed_t& item) :
             writer (queue),
             func (__job<Functor>::functor (functor)),
             batch_size (__batch_size<passed_t> (item)) { }

           void execute () {
             size_t count = 0;
             auto out = writer.placeholder();
             bool stop = false;
             do {
               out->item.resize (batch_size);
               for (size_t n = 0; n < batch_size; ++n) {
                 if (!func (out->item[n])) {
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
         struct __Pipe<__Ordered<__Batch<Item1>>,Functor,__Ordered<__Batch<Item2>>> {
           MEMALIGN(__Pipe<__Ordered<__Batch<Item1>>,Functor,__Ordered<__Batch<Item2>>>)

           using queued1_t = __Ordered<vector<Item1>>;
           using queued2_t = __Ordered<vector<Item2>>;
           using passed2_t = __Ordered<__Batch<Item2>>;
           using queue1_t = typename Type<queued1_t>::queue;
           using queue2_t = typename Type<queued2_t>::queue;
           using reader_t = typename Type<queued1_t>::reader;
           using writer_t = typename Type<queued2_t>::writer;
           using functor_t = typename __job<Functor>::member_type;

           reader_t reader;
           writer_t writer;
           functor_t func;
           const size_t batch_size;

           __Pipe (queue1_t& queue_in, Functor& functor, queue2_t& queue_out, const passed2_t& item2) :
             reader (queue_in),
             writer (queue_out),
             func (__job<Functor>::functor (functor)),
             batch_size (__batch_size<passed2_t> (item2)) { }

           void execute () {
             auto in = reader.placeholder();
             auto out = writer.placeholder();
             while (in.read()) {
               out->item.resize (in->item.size());
               size_t k = 0;
               for (size_t n = 0; n < in->item.size(); ++n) {
                 if (func (in->item[n], out->item[k]))
                   ++k;
               }
               out->item.resize (k);
               out->index = in->index;
               if (!out.write())
                 return;
             }
           }

         };






       template <class Item, class Functor>
         struct __Sink<__Ordered<__Batch<Item>>,Functor> {
           MEMALIGN(__Sink<__Ordered<__Batch<Item>>,Functor>)

           using queued_t = __Ordered<vector<Item>>;
           using queue_t = typename Type<queued_t>::queue;
           using reader_t = typename Type<queued_t>::reader;
           using functor_t = typename __job<Functor>::member_type;

           reader_t reader;
           functor_t func;

           __Sink (queue_t& queue, Functor& functor) :
             reader (queue),
             func (__job<Functor>::functor (functor)) { }

           void execute () {
             size_t expected = 0;
             auto in = reader.placeholder();
             std::set<queued_t*,CompareItems> buffer;
             while (in.read()) {
               if (in->index > expected) {
                 buffer.emplace (in.stash());
                 continue;
               }
               for (size_t n = 0; n < in->item.size(); ++n)
                 if (!func (in->item[n]))
                   return;
               ++expected;
               while (!buffer.empty() && (*buffer.begin())->index <= expected) {
                 for (size_t n = 0; n < (*buffer.begin())->item.size(); ++n)
                   if (!func ((*buffer.begin())->item[n]))
                     return;
                 in.recycle (*buffer.begin());
                 buffer.erase (buffer.begin());
                 ++expected;
               }
             }
           }
         };





    }




       template <class Source, class Item1, class Pipe, class Item2, class Sink>
         inline void run_ordered_queue (
             Source&& source,
             const Item1& item1,
             Pipe&& pipe,
             const Item2& item2,
             Sink&& sink,
             size_t capacity = MRTRIX_QUEUE_DEFAULT_CAPACITY)
         {
           static_assert (job_is_single_threaded<Source>::value && job_is_single_threaded<Sink>::value,
               "run_ordered_queue can only run with single-threaded source & sink");

           if (__batch_size<Item1>(item1) != __batch_size<Item2>(item2))
             throw Exception ("Thread::run_ordered_queue must be run with matching batch sizes across all stages");

           run_queue (
               std::move (source),
               __Ordered<Item1>(item1),
               std::move (pipe),
               __Ordered<Item2>(item2),
               std::move (sink),
               capacity);
         }




       template <class Source, class Item1, class Pipe1, class Item2, class Pipe2, class Item3, class Sink>
         inline void run_ordered_queue (
             Source&& source,
             const Item1& item1,
             Pipe1&& pipe1,
             const Item2& item2,
             Pipe2&& pipe2,
             const Item3& item3,
             Sink&& sink,
             size_t capacity = MRTRIX_QUEUE_DEFAULT_CAPACITY)
         {
           static_assert (job_is_single_threaded<Source>::value && job_is_single_threaded<Sink>::value,
               "run_ordered_queue can only run with single-threaded source & sink");

           if (__batch_size<Item1>(item1) != __batch_size<Item2>(item2) ||
               __batch_size<Item1>(item1) != __batch_size<Item3>(item3))
             throw Exception ("Thread::run_ordered_queue must be run with matching batch sizes across all stages");

           run_queue (
               std::move (source),
               __Ordered<Item1>(item1),
               std::move (pipe1),
               __Ordered<Item2>(item2),
               std::move (pipe2),
               __Ordered<Item3>(item3),
               std::move (sink),
               capacity);
         }


  }
}

#endif

