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

#include <unistd.h>

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
    Consumer (Thread::Queue<Item>& queue, const std::string& description = "unnamed") : reader (queue), desc (description) { }
    const std::string& name () { return (desc); }
    void execute () {
      Item* item = NULL;
      size_t count = 0;
      Thread::Queue<Item>::Read read (reader);
      while ((item = read())) {
        //std::cout << item->orig << " => " << item->processed << "\n"; 
        delete item;
        ++count;
      }
      print ("consumer count = " + str(count) + "\n");
    }
  private:
    Thread::Queue<Item>::Reader reader;
    std::string desc;
};

class Transformer {
  public:
    Transformer (Thread::Queue<float>& queue_in, Thread::Queue<Item>& queue_out, const std::string& description = "unnamed") : 
      reader (queue_in), writer (queue_out), desc (description) { }
    //Transformer (const Transformer& T) : reader (T.reader), writer (T.writer), desc (T.desc) { }
    const std::string& name () { return (desc); }
    void execute () {
      Item* item;
      float* value;
      size_t count = 0;
      Thread::Queue<float>::Read read (reader);
      Thread::Queue<Item>::Write write (writer);
      do {
        if (!(value = read())) break;
        item = new Item;
        item->orig = *value;
        item->processed = Math::pow2(item->orig);
        delete value;
        ++count;
        usleep (100);
        //print ("[" + name() + "] " + str(item->orig) + " -> " + str(item->processed) + "\n");
      } while (write (item)); 
      print (name() + " count = " + str(count) + "\n");
    }
  private:
    Thread::Queue<float>::Reader reader;
    Thread::Queue<Item>::Writer writer;
    std::string desc;
};

EXECUTE {
  Thread::init();
  VAR (Thread::number());

  Thread::Queue<float> queue1 ("first queue");
  Thread::Queue<Item> queue2 ("second queue");

  Consumer consumer (queue2, "consumer");
  Transformer transformer (queue1, queue2, "func");
  Thread::Array<Transformer> transformer_list (transformer);

  Math::RNG rng;
  Thread::Queue<float>::Writer writer (queue1);

  queue1.status();
  queue2.status();

  Thread::Exec consumer_thread (consumer, consumer.name());
  Thread::Exec func_threads (transformer_list, transformer.name());

  float* value;
  size_t count = 0;
  const size_t N = 100000;
  ProgressBar::init (N, "testing threads...");
  Thread::Queue<float>::Write write (writer);
  do {
    value = new float;
    *value = rng.uniform();
    ++count;
    ProgressBar::inc();
  } while (write (value) && count < N);
  ProgressBar::done();
  print ("producer count = " + str(count) + "\n");

}

