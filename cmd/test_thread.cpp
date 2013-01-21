#include "app.h"
#include "timer.h"
#include "file/config.h"
#include "progressbar.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "math/rng.h"


MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "this is used to test stuff.";
}




class Item
{
  public:
    size_t n;
};

class Source
{
  public:
    Source () : count (0), max_count (2000000) { }
    size_t count, max_count;
    bool operator() (Item& item) {
      item.n = count++;
      return count < max_count;
    }
};


class Process 
{
  public:
    bool operator() (const Item& in, Item& out) {
      out.n = in.n % 128;
      return true;
    }
};

class Sink
{
  public:
    Sink () : count (0), total_count (0) { }
    size_t count, total_count;
    bool operator() (const Item& item) {
      count += item.n;
      ++total_count;
      return true;
    }
};



void run ()
{
  Source source;
  Process process;
  Sink sink;


  Timer timer;
  Thread::run_queue_threaded_pipe (source, Item(), process, Item(), sink);

  double time_not_batched = timer.elapsed();
  VAR (time_not_batched);
  VAR (sink.count);
  VAR (sink.total_count);

  Source source2;
  Sink sink2;
  timer.start();

  Thread::run_batched_queue_threaded_pipe (source2, Item(), 1024, process, Item(), 1024, sink2);

  double time_batched = timer.elapsed();
  VAR (time_batched);
  VAR (timer.elapsed());
  VAR (sink2.count);
  VAR (sink2.total_count);

  VAR (time_not_batched / time_batched);
}
