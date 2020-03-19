/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#ifdef MRTRIX_MACOSX
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

#include "command.h"
#include "exception.h"
#include "ordered_thread_queue.h"
#include "timer.h"


using namespace MR;
using namespace App;


#ifdef MRTRIX_MACOSX
#define MALLOC_SIZE(ptr) malloc_size(ptr)
#else
#define MALLOC_SIZE(ptr) malloc_usable_size(ptr)
#endif


#define START(msg) { Timer timer; CONSOLE(std::string("testing ") + msg + " queue..."); memory_usage = peak_memory_usage = num_items = 0

#define END(enforce) CONSOLE("done in " + str(timer.elapsed(), 4) + " seconds"); \
  if (sample_size_received != sample_size) throw Exception("sample size mismatch"); \
  if (out_of_order) { \
    if (enforce) throw Exception ("order mismatch"); \
    else CONSOLE ("order mismatch"); \
  } else { \
    CONSOLE ("order correct"); \
  } } \
  CONSOLE ("peak memory usage = " + str(peak_memory_usage/1024) + " kb (leaked: " + str(memory_usage/1024 ) + " kb)"); \
  CONSOLE ("allocated a total of " + str(num_items) + " items"); \
  std::cerr << "\n"




// to track memory usage:

size_t memory_usage = 0;
size_t peak_memory_usage = 0;
size_t num_items;
std::mutex malloc_mutex;

void * operator new(decltype(sizeof(0)) n)
{
  std::lock_guard<std::mutex> lock (malloc_mutex);
  void * p = malloc (n);
  memory_usage += MALLOC_SIZE(p);
  peak_memory_usage = std::max (memory_usage, peak_memory_usage);
  return p;
}
void operator delete(void * p) noexcept
{
  std::lock_guard<std::mutex> lock (malloc_mutex);
  memory_usage -= MALLOC_SIZE(p);
  free (p);
}




void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";
  SYNOPSIS = "test Thread::run_ordered_queue() functions";
  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}



struct Item {
  Item() { num_items++; }
  size_t value;
};

const size_t sample_size = 1e6;
size_t sample_size_received, out_of_order;


struct SourceFunctor { NOMEMALIGN
  SourceFunctor () : count (0), value (0) { }
  ~SourceFunctor () { std::cerr << "source sent " << count << " items\n"; }
  bool operator() (Item& item) {
    if (++value > sample_size) return false;
    ++count;
    item.value = value;
    return true;
  };
  size_t count;
  size_t value;
};

struct PipeFunctor { NOMEMALIGN
  bool operator() (const Item& in, Item& out) {
    out = in;
    return true;
  }
};

struct SinkFunctor { NOMEMALIGN
  SinkFunctor () : value (0) { sample_size_received = 0; out_of_order = 0; }
  ~SinkFunctor () { std::cerr << "received " << sample_size_received << " items with " << out_of_order << " items out of order\n"; }
  bool operator() (const Item& item) {
    ++sample_size_received;
    if (item.value <= value)
      ++out_of_order;
    value = item.value;
    return true;
  }
  size_t value;
};


void run ()
{
  using namespace Thread;

  START ("regular 2-stage");
  run_queue (
      SourceFunctor(),
      Item(),
      SinkFunctor());
  END(true);

  START ("batched 2-stage");
  run_queue (
      SourceFunctor(),
      batch (Item()),
      SinkFunctor());
  END(true);


  START ("regular 3-stage");
  run_queue (
      SourceFunctor(),
      Item(),
      multi (PipeFunctor()),
      Item(),
      SinkFunctor());
  END(false);

  START ("batched-unbatched 3-stage");
  run_queue (
      SourceFunctor(),
      batch (Item()),
      multi (PipeFunctor()),
      Item(),
      SinkFunctor());
  END(false);

  START ("unbatched-batched 3-stage");
  run_queue (
      SourceFunctor(),
      Item(),
      multi (PipeFunctor()),
      batch (Item()),
      SinkFunctor());
  END(false);

  START ("batched-batched regular 3-stage");
  run_queue (
      SourceFunctor(),
      batch (Item()),
      multi (PipeFunctor()),
      batch (Item()),
      SinkFunctor());
  END(false);


  START ("regular 4-stage");
  run_queue (
      SourceFunctor(),
      Item(),
      multi (PipeFunctor()),
      Item(),
      multi (PipeFunctor()),
      Item(),
      SinkFunctor());
  END(false);

  START ("batched-unbatched-unbatched 4-stage");
  run_queue (
      SourceFunctor(),
      batch (Item()),
      multi (PipeFunctor()),
      Item(),
      multi (PipeFunctor()),
      Item(),
      SinkFunctor());
  END(false);

  START ("unbatched-batched-unbatched 4-stage");
  run_queue (
      SourceFunctor(),
      Item(),
      multi (PipeFunctor()),
      batch (Item()),
      multi (PipeFunctor()),
      Item(),
      SinkFunctor());
  END(false);

  START ("unbatched-unbatched-batched regular 4-stage");
  run_queue (
      SourceFunctor(),
      Item(),
      multi (PipeFunctor()),
      Item(),
      multi (PipeFunctor()),
      batch (Item()),
      SinkFunctor());
  END(false);



  START ("ordered unbatched 3-stage");
  run_ordered_queue (
      SourceFunctor(),
      Item(),
      multi (PipeFunctor()),
      Item(),
      SinkFunctor());
  END(true);

  START ("ordered batched-batched 3-stage");
  run_ordered_queue (
      SourceFunctor(),
      batch (Item()),
      multi (PipeFunctor()),
      batch (Item()),
      SinkFunctor());
  END(true);

  START ("unbatched 4-stage");
  run_ordered_queue (
      SourceFunctor(),
      Item(),
      multi (PipeFunctor()),
      Item(),
      multi (PipeFunctor()),
      Item(),
      SinkFunctor());
  END(true);


  START ("ordered batched-batched-batched 4-stage");
  run_ordered_queue (
      SourceFunctor(),
      batch (Item()),
      multi (PipeFunctor()),
      batch (Item()),
      multi (PipeFunctor()),
      batch (Item()),
      SinkFunctor());
  END(true);

}
