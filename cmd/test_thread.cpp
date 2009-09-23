/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "app.h"
#include "thread.h"
#include "math/rng.h"

using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "this is used to test stuff.",
  NULL
};


ARGUMENTS = { Argument::End }; 
OPTIONS = { Option::End };

class ProcessedItem {
  public:
    float orig, processed;
};

class Output {
  public:
    bool process (ProcessedItem item) {
      std::cout << item.orig << " => " << item.processed << "\n"; 
      return (false);
    }
};

class Functor {
  public:
    Functor (Thread::Serial<Output,ProcessedItem>& queue) : Q (queue) { }
    bool process (float value) { 
      ProcessedItem item;
      item.orig = value;
      item.processed = Math::pow2(value);
      if (Q.push (item)) return (true);
      return (false); 
    }
  private:
    Thread::Serial<Output,ProcessedItem>& Q;
};

EXECUTE {
  VAR (Thread::num_cores());

  Output output_func;
  Thread::Serial<Output,ProcessedItem> output_queue (output_func, "output thread");

  {
    Functor process_func (output_queue);
    Thread::Parallel<Functor,float> process_queue (process_func, "process thread", 2);

    Math::RNG rng;
    for (size_t i = 0; i < 200; ++i)
      process_queue.push (rng.uniform());
    process_queue.end();
  }
  TEST;

  output_queue.end();
}

