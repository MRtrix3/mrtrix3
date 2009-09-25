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

class Item {
  public:
    float orig, processed;
};

class Consumer : public Thread::Exec {
  public:
    Consumer (Thread::Queue<Item>& queue) : Thread::Exec ("consumer"), in (queue) { in.open(); }
    void execute () {
      Item* item = NULL;
      size_t count = 0;
      while ((item = in.pop())) {
        //std::cout << item->orig << " => " << item->processed << "\n"; 
        delete item;
        ++count;
      }
      in.close();
      VAR (count);
    }
  private:
    Thread::Queue<Item>::Pop in;
};

class Transformer : public Thread::Exec {
  public:
    Transformer (Thread::Queue<float>& queue_in, Thread::Queue<Item>& queue_out) : 
      Thread::Exec ("transformer"), in (queue_in), out (queue_out) { 
      in.open(); 
      out.open(); 
    }
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
      } while (out.push (item)); 
      in.close();
      out.close();
      VAR (count);
    }
  private:
    Thread::Queue<float>::Pop in;
    Thread::Queue<Item>::Push out;
};

EXECUTE {
  VAR (Thread::num_cores());

  Thread::Queue<float> queue1 ("first queue");
  Thread::Queue<Item> queue2 ("second queue");

  Consumer consumer (queue2);
  Transformer func1 (queue1, queue2);
  Transformer func2 (queue1, queue2);
  Transformer func3 (queue1, queue2);
  Transformer func4 (queue1, queue2);

  Math::RNG rng;
  Thread::Queue<float>::Push out (queue1);
  out.open();

  queue1.status();
  queue2.status();

  consumer.start();
  func1.start();
  func2.start();
  func3.start();
  func4.start();

  float* value;
  size_t count = 0;
  do {
    value = new float;
    *value = rng.uniform();
    ++count;
  } while (out.push (value) && count < 1000000);
  out.close();
  VAR (count);

  consumer.finish();
  func1.finish();
  func2.finish();
  func3.finish();
  func4.finish();
}

