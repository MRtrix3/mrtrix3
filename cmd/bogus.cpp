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

#include "command.h"
#include "debug.h"
#include "timer.h"
#include "math/math.h"
#include "thread/queue.h"


using namespace MR;
using namespace App;



void usage () {

  ARGUMENTS
    + Argument ("num", "number of items").type_integer();
}



typedef double Item;

class Source 
{
  public:
    Source (size_t max) : max (max), count (0) { }

    bool operator() (Item& item) 
    {
      item = Math::cos (double (count));
      return ++count < max;
    }

  private:
    const size_t max;
    size_t count;
};


class Pipe 
{
  public:
    bool operator() (const Item& in, Item& out) {
      out = Math::cosh (in);
      return true;
    }
};

class Sink 
{
  public:
    Sink (double& grand_total) :
      total (0.0),
      grand_total (grand_total) { }

    ~Sink () 
    {
      grand_total += total;
    }

    bool operator() (const Item& item)
    {
      total += item;
      return true;
    }

  private:
    double total;
    double& grand_total;
};

void run () 
{
  using namespace Thread;
  double total;
  Timer timer;

  size_t num = argument[0];

  total = 0.0; {
    Source source (num); Sink sink (total); timer.start();
    run_queue (source, Item(), sink);
  } std::cout << "source->sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Sink sink (total); timer.start();
    run_queue (source, batch(Item()), sink);
  } std::cout << "source=>sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Sink sink (total); timer.start();
    run_queue (multi(source), Item(), sink);
  } std::cout << "[source]->sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Sink sink (total); timer.start();
    run_queue (multi(source), batch(Item()), sink);
  } std::cout << "[source]=>sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Sink sink (total); timer.start();
    run_queue (source, Item(), multi(sink));
  } std::cout << "source->[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Sink sink (total); timer.start();
    run_queue (source, batch (Item()), multi(sink));
  } std::cout << "source=>[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Sink sink (total); timer.start();
    run_queue (multi(source), Item(), multi(sink));
  } std::cout << "[source]->[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Sink sink (total); timer.start();
    run_queue (multi(source), batch (Item()), multi(sink));
  } std::cout << "[source]=>[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();







  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, Item(), pipe, Item(), sink);
  } std::cout << "source->pipe->sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, batch (Item()), pipe, Item(), sink);
  } std::cout << "source=>pipe->sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, Item(), pipe, batch (Item()), sink);
  } std::cout << "source->pipe=>sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, batch (Item()), pipe, batch (Item()), sink);
  } std::cout << "source=>pipe=>sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();



  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), Item(), pipe, Item(), sink);
  } std::cout << "[source]->pipe->sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), batch (Item()), pipe, Item(), sink);
  } std::cout << "[source]=>pipe->sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), Item(), pipe, batch (Item()), sink);
  } std::cout << "[source]->pipe=>sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), batch (Item()), pipe, batch (Item()), sink);
  } std::cout << "[source]=>pipe=>sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();





  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, Item(), multi(pipe), Item(), sink);
  } std::cout << "source->[pipe]->sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, batch (Item()), multi(pipe), Item(), sink);
  } std::cout << "source=>[pipe]->sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, Item(), multi(pipe), batch (Item()), sink);
  } std::cout << "source->[pipe]=>sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, batch (Item()), multi(pipe), batch (Item()), sink);
  } std::cout << "source=>[pipe]=>sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();




  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, Item(), pipe, Item(), multi(sink));
  } std::cout << "source->pipe->[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, batch (Item()), pipe, Item(), multi(sink));
  } std::cout << "source=>pipe->[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, Item(), pipe, batch (Item()), multi(sink));
  } std::cout << "source->pipe=>[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, batch (Item()), pipe, batch (Item()), multi(sink));
  } std::cout << "source=>pipe=>[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();



  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), Item(), multi(pipe), Item(), sink);
  } std::cout << "[source]->[pipe]->sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), batch (Item()), multi(pipe), Item(), sink);
  } std::cout << "[source]=>[pipe]->sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), Item(), multi(pipe), batch (Item()), sink);
  } std::cout << "[source]->[pipe]=>sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), batch (Item()), multi(pipe), batch (Item()), sink);
  } std::cout << "[source]=>[pipe]=>sink : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();



  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), Item(), pipe, Item(), multi(sink));
  } std::cout << "[source]->pipe->[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), batch (Item()), pipe, Item(), multi(sink));
  } std::cout << "[source]=>pipe->[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), Item(), pipe, batch (Item()), multi(sink));
  } std::cout << "[source]->pipe=>[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), batch (Item()), pipe, batch (Item()), multi(sink));
  } std::cout << "[source]=>pipe=>[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();


  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, Item(), multi(pipe), Item(), multi(sink));
  } std::cout << "source->[pipe]->[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, batch (Item()), multi(pipe), Item(), multi(sink));
  } std::cout << "source=>[pipe]->[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, Item(), multi(pipe), batch (Item()), multi(sink));
  } std::cout << "source->[pipe]=>[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (source, batch (Item()), multi(pipe), batch (Item()), multi(sink));
  } std::cout << "source=>[pipe]=>[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();



  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), Item(), multi(pipe), Item(), multi(sink));
  } std::cout << "[source]->[pipe]->[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), batch (Item()), multi(pipe), Item(), multi(sink));
  } std::cout << "[source]=>[pipe]->[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), Item(), multi(pipe), batch (Item()), multi(sink));
  } std::cout << "[source]->[pipe]=>[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

  total = 0; {
    Source source (num); Pipe pipe; Sink sink (total); timer.start();
    run_queue (multi(source), batch (Item()), multi(pipe), batch (Item()), multi(sink));
  } std::cout << "[source]=>[pipe]=>[sink] : " << timer.elapsed() << ", total = " << total << "\n"; std::cout.flush();

}
