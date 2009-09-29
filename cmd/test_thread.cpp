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
#include "progressbar.h"
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

class Item {
  public:
    float orig, processed;
};

class Consumer {
  public:
    Consumer (Thread::Queue<Item>& queue, const std::string& description = "unnamed") : in (queue), desc (description) { }
    const std::string& name () { return (desc); }
    void execute () {
      Item* item = NULL;
      size_t count = 0;
      while ((item = in.pop())) {
        //std::cout << item->orig << " => " << item->processed << "\n"; 
        delete item;
        ++count;
      }
      in.close();
      print ("consumer count = " + str(count) + "\n");
    }
  private:
    Thread::Queue<Item>::Pop in;
    std::string desc;
};

class Transformer {
  public:
    Transformer (Thread::Queue<float>& queue_in, Thread::Queue<Item>& queue_out, const std::string& description = "unnamed") : 
      in (queue_in), out (queue_out), desc (description) { }
    const std::string& name () { return (desc); }
    void execute () {
      Item* item;
      float* value;
      size_t count = 0;
      do {
        if (!(value = in.pop())) break;
        item = new Item;
        item->orig = *value;
        item->processed = Math::pow2(item->orig);
        delete value;
        ++count;
        //print ("[" + name() + "] " + str(item->orig) + " -> " + str(item->processed) + "\n");
      } while (out.push (item)); 
      in.close();
      out.close();
      print (name() + " count = " + str(count) + "\n");
    }
  private:
    Thread::Queue<float>::Pop in;
    Thread::Queue<Item>::Push out;
    std::string desc;
};

EXECUTE {
  Thread::init();
  VAR (Thread::num_cores());

  Thread::Queue<float> queue1 ("first queue");
  Thread::Queue<Item> queue2 ("second queue");

  Consumer consumer (queue2, "consumer");
  Transformer func1 (queue1, queue2, "func1");
  Transformer func2 (queue1, queue2, "func2");
  Transformer func3 (queue1, queue2, "func3");
  Transformer func4 (queue1, queue2, "func4");

  Math::RNG rng;
  Thread::Queue<float>::Push out (queue1);

  queue1.status();
  queue2.status();

  Thread::Exec consumer_thread (consumer, consumer.name());
  Thread::Exec func1_thread (func1, func1.name());
  Thread::Exec func2_thread (func2, func2.name());
  Thread::Exec func3_thread (func3, func3.name());
  Thread::Exec func4_thread (func4, func4.name());

  float* value;
  size_t count = 0;
  const size_t N = 100000;
  ProgressBar::init (N, "testing threads...");
  do {
    value = new float;
    *value = rng.uniform();
    ++count;
    ProgressBar::inc();
  } while (out.push (value) && count < N);
  ProgressBar::done();
  out.close();
  print ("producer count = " + str(count) + "\n");

}

