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
#include "thread/exec.h"
#include "thread/queue.h"
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

class ItemAllocator {
  public:
    Item* alloc () { Item* p = new Item; p->orig = p->processed = NAN; return (p); }
    void reset (Item* p) { p->orig = p->processed = NAN; }
    void dealloc (Item* p) { delete p; }
};

typedef Thread::Queue<Item,ItemAllocator> ItemQueue;
typedef Thread::Queue<float> FloatQueue;



class Consumer {
  public:
    Consumer (ItemQueue& queue, const std::string& description = "unnamed") : reader (queue), desc (description) { }
    const std::string& name () { return (desc); }
    void execute () {
      size_t count = 0;
      ItemQueue::Reader::Item item (reader);
      while (item.read()) {
        //std::cout << item->orig << " => " << item->processed << "\n"; 
        ++count;
      }
      print ("consumer count = " + str(count) + "\n");
    }
  private:
    ItemQueue::Reader reader;
    std::string desc;
};



class Processor {
  public:
    Processor (FloatQueue& queue_in, ItemQueue& queue_out, const std::string& description = "unnamed") : 
      reader (queue_in), writer (queue_out), desc (description) { }
    const std::string& name () { return (desc); }
    void execute () {
      size_t count = 0;
      FloatQueue::Reader::Item in (reader);
      ItemQueue::Writer::Item out (writer);
      do {
        if (!in.read()) break;
        out->orig = *in;
        out->processed = Math::pow2(out->orig);
        ++count;
        //print ("[" + name() + "] " + str(out->orig) + " -> " + str(out->processed) + "\n");
      } while (out.write()); 
      print (name() + " count = " + str(count) + "\n");
    }
  private:
    FloatQueue::Reader reader;
    ItemQueue::Writer writer;
    std::string desc;
};

EXECUTE {
  Thread::init();

  FloatQueue queue1 ("first queue");
  ItemQueue queue2 ("second queue");

  Consumer consumer (queue2, "consumer");
  Processor processor (queue1, queue2, "processor");
  Thread::Array<Processor> transformer_list (processor);

  Math::RNG rng;
  FloatQueue::Writer writer (queue1);

  queue1.status();
  queue2.status();

  Thread::Exec consumer_thread (consumer, consumer.name());
  Thread::Exec func_threads (transformer_list, processor.name());

  size_t count = 0;
  const size_t N = 100000;
  ProgressBar::init (N, "testing threads...");
  FloatQueue::Writer::Item value (writer);
  do {
    *value = rng.uniform();
    ++count;
    ProgressBar::inc();
  } while (value.write() && count < N);
  ProgressBar::done();
  print ("producer count = " + str(count) + "\n");

}

