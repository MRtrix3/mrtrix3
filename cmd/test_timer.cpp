#include "command.h"
#include "timer.h"

#include <ctime>
#include <unistd.h>


using namespace MR;
using namespace App;

void usage () 
{
  DESCRIPTION
    + "test timer interface";

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}

void run () 
{

  Timer timer;

  CONSOLE ("printing Timer::current_time() at 10ms intervals:");
  for (size_t n = 0; n < 10; ++n) {
    CONSOLE ("  current timestamp: " + str(Timer::current_time(), 16)
        + " (from C time(): " + str(time(NULL)) + ")");
    usleep (10000);
  }

  CONSOLE ("execution took " + str(timer.elapsed()) + " seconds");
  timer.start();

  CONSOLE ("testing IntervalTimer with 10 x 0.2s intervals:");
  IntervalTimer itimer (0.2);
  for (size_t n = 0; n < 10; ++n) {
    while (!itimer);
    CONSOLE ("  tick");
  }

  CONSOLE ("execution took " + str(timer.elapsed()) + " seconds");
}

