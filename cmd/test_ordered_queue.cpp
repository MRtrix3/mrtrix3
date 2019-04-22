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

#include "command.h"
#include "ordered_thread_queue.h"


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";
  SYNOPSIS = "test Thread::run_ordered_queue() functions";
  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}


using Item = size_t;


struct SourceFunctor { NOMEMALIGN
  SourceFunctor () : count (0), value (0) { }
  ~SourceFunctor () { std::cerr << "source sent " << count << " items\n"; }
  bool operator() (Item& item) {
    if (++value > 1e4) return false;
    ++count;
    item = value;
    return true;
  };
  size_t count;
  Item value;
};

struct PipeFunctor { NOMEMALIGN
  bool operator() (const Item& in, Item& out) {
    out = in;
    return true;
  }
  double previous;
};

struct SinkFunctor { NOMEMALIGN
  SinkFunctor () : value (0), failure (0), count (0) { }
  ~SinkFunctor () { std::cerr << "received " << count << " items with " << failure << " items out of order\n"; }
  bool operator() (const Item& item) {
    ++count;
    if (item <= value)
      ++failure;
    value = item;
    return true;
  }
  Item value;
  size_t failure, count;
};


void run ()
{
  using namespace Thread;

  CONSOLE ("starting regular 2-stage queue...");
  run_queue (
      SourceFunctor(),
      Item(),
      SinkFunctor());
  CONSOLE ("done...");

  CONSOLE ("starting batched 2-stage queue...");
  run_queue (
      SourceFunctor(),
      batch (Item()),
      SinkFunctor());
  CONSOLE ("done...");


  CONSOLE ("starting regular 3-stage queue...");
  run_queue (
      SourceFunctor(),
      Item(),
      multi (PipeFunctor()),
      Item(),
      SinkFunctor());
  CONSOLE ("done...");

  CONSOLE ("starting batched-unbatched 3-stage queue...");
  run_queue (
      SourceFunctor(),
      batch (Item()),
      multi (PipeFunctor()),
      Item(),
      SinkFunctor());
  CONSOLE ("done...");

  CONSOLE ("starting unbatched-batched 3-stage queue...");
  run_queue (
      SourceFunctor(),
      Item(),
      multi (PipeFunctor()),
      batch (Item()),
      SinkFunctor());
  CONSOLE ("done...");

  CONSOLE ("starting batched-batched regular 3-stage queue...");
  run_queue (
      SourceFunctor(),
      batch (Item()),
      multi (PipeFunctor()),
      batch (Item()),
      SinkFunctor());
  CONSOLE ("done...");


  CONSOLE ("starting regular 4-stage queue...");
  run_queue (
      SourceFunctor(),
      Item(),
      multi (PipeFunctor()),
      Item(),
      multi (PipeFunctor()),
      Item(),
      SinkFunctor());
  CONSOLE ("done...");

  CONSOLE ("starting batched-unbatched-unbatched 4-stage queue...");
  run_queue (
      SourceFunctor(),
      batch (Item()),
      multi (PipeFunctor()),
      Item(),
      multi (PipeFunctor()),
      Item(),
      SinkFunctor());
  CONSOLE ("done...");

  CONSOLE ("starting unbatched-batched-unbatched 4-stage queue...");
  run_queue (
      SourceFunctor(),
      Item(),
      multi (PipeFunctor()),
      batch (Item()),
      multi (PipeFunctor()),
      Item(),
      SinkFunctor());
  CONSOLE ("done...");

  CONSOLE ("starting unbatched-unbatched-batched regular 4-stage queue...");
  run_queue (
      SourceFunctor(),
      Item(),
      multi (PipeFunctor()),
      Item(),
      multi (PipeFunctor()),
      batch (Item()),
      SinkFunctor());
  CONSOLE ("done...");


}
